//
// Created by mateu on 06.04.2026.
//

#pragma once

#include <constraint_evaluator.h>
#include <constraints.h>
#include <data_models.h>
#include <time_table_problem.h>
#include <time_table_state.h>

#include <map>
#include <utility>
#include <variant>
#include <vector>

// IntAbsencePolicy — drop-in replacement for MinimizeTotalAbsenceConstraint
// that supports fast partial evaluation.
//
// Full penalty (same semantics as the original):
//   +1 per unattended (assigned but absent) class
//   +1 per pair of attending classes whose time slots overlap
//
// Partial evaluation strategy:
//   When class_id is placed, count only NEW violations introduced by it:
//     - if unattended:  +1  (for this class)
//     - if attending:   count precomputed overlapping pairs (other_id, other_group)
//                       where other_id < class_id and other is currently attending
//
//   Summing over all placed classes gives the same total as the full penalty
//   because every overlapping pair (i, j) with i < j is attributed to j.
//
// Precomputation at construction:
//   overlap_map_[(class_id, group)] = list of (other_id, other_group) with
//   other_id < class_id that overlap this slot.  Built once in O(n^2 * g^2).
struct IntAbsencePolicy
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int id = 0;
    constraints::ConstraintType type = constraints::ConstraintType::MinimizeTotalAbsence;

    IntAbsencePolicy() = default;

    // Substitutable factory — constructs from the matching MinimizeTotalAbsenceConstraint.
    static IntAbsencePolicy make(const solver_models::ConstraintVariant& c,
                                 const TimeTableProblem& problem)
    {
        const auto& base = std::get<solver_models::MinimizeTotalAbsenceConstraint>(c);
        IntAbsencePolicy p;
        p.id       = base.id;
        p.sequence = base.sequence;
        p.hard     = base.hard;
        p.weight   = base.weight;
        p.slack    = base.slack;
        p.precompute(problem);
        return p;
    }

    void precompute(const TimeTableProblem& problem)
    {
        overlap_map_.clear();
        const int n = static_cast<int>(problem.class_size());

        for (int ci = 0; ci < n; ++ci)
        {
            const int max_gi = problem.get_max_group(ci);
            for (int gi = 1; gi <= max_gi; ++gi)
            {
                const auto& a = problem.get_group(ci, gi);
                for (int cj = 0; cj < ci; ++cj)         // only lower-index classes
                {
                    const int max_gj = problem.get_max_group(cj);
                    for (int gj = 1; gj <= max_gj; ++gj)
                    {
                        const auto& b = problem.get_group(cj, gj);
                        if (time_overlaps(a, b))
                        {
                            overlap_map_[{ci, gi}].push_back({cj, gj});
                        }
                    }
                }
            }
        }
    }

    // -------------------- Evaluatable interface --------------------

    [[nodiscard]] double penalty(const TimeTableState& state,
                                 const TimeTableProblem& problem) const
    {
        double count = 0.0;
        for (int cid = 0; cid < static_cast<int>(state.size()); ++cid)
        {
            if (state.is_unattended(cid))
            {
                count += 1;
                continue;
            }
            if (!state.is_attended(cid))
            {
                continue;
            }

            for (int class_a_id = cid + 1; class_a_id < static_cast<int>(state.size()); ++class_a_id)
            {
                if (!state.is_attended(class_a_id))
                {
                    continue;
                }

                const auto& class_a = problem.get_group(class_a_id, state.get_raw_group(class_a_id));
                const auto& class_b = problem.get_group(cid, state.get_raw_group(cid));
                if (time_overlaps(class_a, class_b))
                {
                    count += 1;
                }
            }
        }
        return count;
    }

    [[nodiscard]] double evaluate(const TimeTableState& state,
                                  const TimeTableProblem& problem) const
    {
        return weight * penalty(state, problem);
    }

    [[nodiscard]] bool is_satisfied(const TimeTableState& state,
                                    const TimeTableProblem& problem) const
    {
        for (int cid = 0; cid < static_cast<int>(state.size()); ++cid)
        {
            if (!state.is_assigned(cid)) continue;
            if (state.is_unattended(cid)) return false;
            // any overlap with a lower-indexed attending class is a violation
            const int g = state.get_raw_group(cid);
            const auto it = overlap_map_.find({cid, g});
            if (it == overlap_map_.end()) continue;
            for (const auto& [oid, og] : it->second)
                if (state.is_attended(oid, og)) return false;
        }
        return true;
    }

    [[nodiscard]] bool is_feasible(const TimeTableState& state,
                                   const TimeTableProblem& problem,
                                   const SequenceContext& context) const
    {
        if (!context.has_score(id)) return true;
        return penalty(state, problem) <= context[id] + slack;
    }

    [[nodiscard]] double lower_bound(const TimeTableState& state,
                                     const TimeTableProblem& problem) const
    {
        return evaluate(state, problem);
    }

private:
    struct OverlapEntry { int class_id; int group; };

    static bool time_overlaps(const solver_models::Class& a,
                               const solver_models::Class& b) noexcept
    {
        if (a.day != b.day)              return false;
        if (!(a.week & b.week).any())    return false;
        return !(b.end_time < a.start_time || a.end_time < b.start_time);
    }

    // Incremental penalty contribution of placing class_id:
    //   +1  if unattended
    //   +N  where N = number of precomputed lower-indexed overlapping pairs
    //              whose partner is currently attending
    [[nodiscard]] double incremental_penalty(const TimeTableState& state,
                                              const int class_id) const
    {
        if (state.is_unattended(class_id)) return 1.0;
        if (!state.is_attended(class_id))  return 0.0;

        const int g  = state.get_raw_group(class_id);
        const auto it = overlap_map_.find({class_id, g});
        if (it == overlap_map_.end()) return 0.0;

        double count = 0.0;
        for (const auto& [oid, og] : it->second)
        {
            if (state.is_attended(oid, og)) count += 1.0;
        }
        return count;
    }

    // (class_id, group) → overlapping (other_id, other_group) pairs
    // where other_id < class_id.  Built once in precompute().
    std::map<std::pair<int,int>, std::vector<OverlapEntry>> overlap_map_;
};

static_assert(policies::Evaluatable<IntAbsencePolicy>,
    "IntAbsencePolicy must satisfy policies::Evaluatable");

static_assert(policies::BoundEstimating<IntAbsencePolicy>,
    "IntAbsencePolicy must satisfy policies::BoundEstimating");

static_assert(policies::Substitutable<IntAbsencePolicy>,
    "IntAbsencePolicy must satisfy policies::Substitutable");

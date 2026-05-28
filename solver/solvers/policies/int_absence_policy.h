//
// Created by mateu on 06.04.2026.
//

#pragma once

#include <constraint_evaluator.h>
#include <constraints.h>
#include <data_models.h>
#include <solver_config.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <traits.h>

#include <map>
#include <set>
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
template<SolverTraitsConcept Traits>
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
        return std::visit([&](const auto& src) -> IntAbsencePolicy<Traits>
        {
            IntAbsencePolicy p;
            p.id       = src.id;
            p.sequence = src.sequence;
            p.hard     = src.hard;
            p.weight   = src.weight;
            p.slack    = src.slack;
            p.precompute(problem);
            return p;
        }, c);
    }

    void precompute(const TimeTableProblem& problem)
    {
        if constexpr (Traits::config.use_simplified_evaluation)
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
                            if (weekly_time_overlaps(a, b))
                            {
                                overlap_map_[{ci, gi}].push_back({cj, gj});
                            }
                        }
                    }
                }
            }
        }
        else
        {
            overlap_map_.clear();
            const int n_classes = static_cast<int>(problem.class_size());

            for (int ci = 0; ci < n_classes; ++ci)
            {
                const int max_gi = problem.get_max_group(ci);
                for (int gi = 1; gi <= max_gi; ++gi)
                {
                    const auto& class_a = problem.get_group(ci, gi);
                    auto& entry = overlap_map_[{ci, gi}];
                    entry.first = static_cast<int>(class_a.sessions.size());

                    for (int cj = 0; cj < n_classes; ++cj)
                    {
                        if (ci == cj) continue;
                        const int max_gj = problem.get_max_group(cj);
                        for (int gj = 1; gj <= max_gj; ++gj)
                        {
                            const auto& class_b = problem.get_group(cj, gj);
                            // Collect dates where class_a and class_b share an overlapping session.
                            // Each date appears at most once — overlap_per_date[d] then counts
                            // distinct overlapping CLASS-GROUP pairs, not session pairs.
                            std::set<int> common_dates;
                            for (const auto& sa : class_a.sessions)
                            {
                                for (const auto& sb : class_b.sessions)
                                {
                                    if (sa.date == sb.date &&
                                        !(sb.end_time < sa.start_time || sa.end_time < sb.start_time))
                                    {
                                        common_dates.insert(sa.date);
                                        break; // one match per sa is enough
                                    }
                                }
                            }
                            if (!common_dates.empty())
                            {
                                entry.second.push_back({cj, gj,
                                    std::vector<int>(common_dates.begin(), common_dates.end())});
                            }
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
        if constexpr (Traits::config.use_simplified_evaluation)
        {
            // Count: +1 per unattended class, +1 per overlapping attending pair.
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

                for (int aid = cid + 1; aid < static_cast<int>(state.size()); ++aid)
                {
                    if (!state.is_attended(aid)) continue;
                    const auto& ca = problem.get_group(cid, state.get_raw_group(cid));
                    const auto& cb = problem.get_group(aid, state.get_raw_group(aid));
                    if (weekly_time_overlaps(ca, cb))
                    {
                        count += 1;
                    }
                }
            }
            return count;
        }
        else
        {
            // Yearly: sum of per-class absence fractions.
            // For each session on date d, attendance = 1 / (1 + oc) where oc is the number
            // of other attending class-group pairs that overlap on date d.
            // Absence per session = oc / (1 + oc).  Computing absence directly (not 1 - attendance)
            // keeps the == 0.0 check stable: sessions with no overlap contribute exactly 0.
            double total_absence = 0.0;
            for (int cid = 0; cid < static_cast<int>(state.size()); ++cid)
            {
                if (!state.is_assigned(cid)) continue;
                if (state.is_unattended(cid)) { total_absence += 1.0; continue; }
                if (!state.is_attended(cid))  { continue; }

                const int g  = state.get_raw_group(cid);
                const auto it = overlap_map_.find({cid, g});
                if (it == overlap_map_.end()) continue;

                const auto& [n_sessions, overlapping] = it->second;
                if (n_sessions == 0) continue;

                // Accumulate per-date overlap counts from currently attending classes.
                std::map<int, int> overlap_per_date;
                for (const auto& ovl : overlapping)
                {
                    if (!state.is_attended(ovl.class_id, ovl.group)) continue;
                    for (int d : ovl.overlapping_dates)
                        ++overlap_per_date[d];
                }

                if (overlap_per_date.empty()) continue; // no overlaps → zero absence

                // Sum oc/(1+oc) over sessions that have at least one overlap.
                const auto& cls = problem.get_group(cid, g);
                double absence = 0.0;
                for (const auto& s : cls.sessions)
                {
                    const auto jt = overlap_per_date.find(s.date);
                    if (jt != overlap_per_date.end())
                    {
                        const int oc = jt->second;
                        absence += static_cast<double>(oc) / static_cast<double>(1 + oc);
                    }
                }
                total_absence += absence / static_cast<double>(n_sessions);
            }
            return total_absence;
        }
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
            if constexpr (Traits::config.use_simplified_evaluation)
            {
                for (const auto& [oid, og] : it->second)
                {
                    if (state.is_attended(oid, og)) return false;
                }
            }
            else
            {
                const auto& [count, overlapping_groups] = it->second;
                for (const auto& [oid, og, _] : overlapping_groups)
                {
                    if (state.is_attended(oid, og)) return false;
                }
            }
        }
        return true;
    }

    [[nodiscard]] bool is_feasible(const TimeTableState& state,
                                   const TimeTableProblem& problem,
                                   const SequenceContext& context) const
    {
        if (!context.has_constraint_score(id)) return true;
        return penalty(state, problem) <= context.get_constraint_score(id) + slack;
    }

    [[nodiscard]] double lower_bound(const TimeTableState& state,
                                     const TimeTableProblem& problem) const
    {
        return evaluate(state, problem);
    }

private:
    struct WeeklyOverlapEntry { int class_id; int group; };
    struct YearlyOverlapEntry { int class_id; int group; std::vector<int> overlapping_dates;};

    static bool weekly_time_overlaps(const solver_models::Class& a,
                                     const solver_models::Class& b) noexcept
    {
        if (a.day != b.day)              return false;
        if (!(a.week & b.week).any())    return false;
        return !(b.end_time < a.start_time || a.end_time < b.start_time);
    }

    // (class_id, group) → overlapping (other_id, other_group) pairs
    // where other_id < class_id.  Built once in precompute().
    using Overlap = std::conditional_t<Traits::config.use_simplified_evaluation,
        std::vector<WeeklyOverlapEntry>, std::pair<int, std::vector<YearlyOverlapEntry>>
    >;
    std::map<std::pair<int,int>, Overlap> overlap_map_;

};

static_assert(policies::Evaluatable<IntAbsencePolicy<SolverTraits>>,
    "IntAbsencePolicy must satisfy policies::Evaluatable");

static_assert(policies::BoundEstimating<IntAbsencePolicy<SolverTraits>>,
    "IntAbsencePolicy must satisfy policies::BoundEstimating");

static_assert(policies::Substitutable<IntAbsencePolicy<SolverTraits>, SolverTraits>,
    "IntAbsencePolicy must satisfy policies::Substitutable");

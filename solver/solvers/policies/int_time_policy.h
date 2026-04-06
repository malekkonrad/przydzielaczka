//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <constraint_evaluator.h>
#include <constraints.h>
#include <time_table_problem.h>
#include <time_table_state.h>

#include <algorithm>
#include <map>
#include <ranges>
#include <utility>
#include <vector>

// IntTimePolicy — a precomputed, drop-in replacement for MinimizeGapsConstraint.
//
// At construction the entire (day, week) → sorted class entries table is built
// once from the problem. penalty() and is_satisfied() then do no sorting and no
// problem.get_group() calls at runtime — they only iterate the precomputed table
// and check state.is_attended() per entry.
//
// Satisfies solver_models::Evaluatable — can be used anywhere a constraint is
// accepted, including as a template argument for PolicyConstraintEvaluator<P>.
//
// Also satisfies PartiallyEvaluatable: the partial_* methods restrict evaluation
// to the (day, week) buckets that contain the given (class_id, group), so the
// backtracking solver can prune after each individual assignment rather than only
// at the leaf.
struct IntTimePolicy
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int min_break{};
    int id = 0;
    constraints::ConstraintType type = constraints::ConstraintType::MinimizeGaps;

    IntTimePolicy() = default;

    // Precomputes the lookup table from the problem.
    // After construction, all evaluation methods ignore the `problem` argument.
    IntTimePolicy(const int sequence, const double weight, const bool hard,
                  const int slack, const int min_break, const int id,
                  const TimeTableProblem& problem)
        : sequence(sequence), weight(weight), hard(hard),
          slack(slack), min_break(min_break), id(id)
    {
        precompute(problem);
    }

    // Substitutable factory — constructs from the matching MinimizeGapsConstraint.
    static IntTimePolicy make(const solver_models::ConstraintVariant& c, const TimeTableProblem& problem)
    {
        return std::visit([&](const auto& src) -> IntTimePolicy {
            IntTimePolicy p;
            p.id       = src.id;
            p.sequence = src.sequence;
            p.hard     = src.hard;
            p.weight   = src.weight;
            p.slack    = src.slack;
            if constexpr (requires { src.min_break; })
                p.min_break = src.min_break;
            p.precompute(problem);
            return p;
        }, c);
    }

    // Can also be called manually after default construction.
    void precompute(const TimeTableProblem& problem)
    {
        lookup_.clear();
        class_group_to_keys_.clear();

        for (int class_id = 0; class_id < static_cast<int>(problem.class_size()); ++class_id)
        {
            const int max_group = problem.get_max_group(class_id);
            for (int group = 1; group <= max_group; ++group)
            {
                const auto& cls = problem.get_group(class_id, group);
                const Entry e{ cls.start_time, cls.end_time, class_id, group };
                if (cls.week.test(0))
                {
                    lookup_[{cls.day, 0}].push_back(e);
                    class_group_to_keys_[{class_id, group}].push_back({cls.day, 0});
                }
                if (cls.week.test(1))
                {
                    lookup_[{cls.day, 1}].push_back(e);
                    class_group_to_keys_[{class_id, group}].push_back({cls.day, 1});
                }
            }
        }
        // Sort each bucket by start_time once — never sorted again at runtime.
        for (auto& entries : lookup_ | std::views::values)
            std::ranges::sort(entries, {}, &Entry::start_time);
    }

    // -------------------- Evaluatable interface --------------------

    [[nodiscard]] double penalty(const TimeTableState& state,
                                 const TimeTableProblem& /*problem*/) const
    {
        double p = 0.0;
        for (const auto& entries : lookup_ | std::views::values)
        {
            int prev_end = -1;
            for (const auto& e : entries)
            {
                if (!state.is_attended(e.class_id, e.group)) continue;
                if (prev_end >= 0)
                {
                    const int gap = e.start_time - prev_end;
                    if (gap > min_break)
                        p += static_cast<double>(gap - min_break);
                }
                prev_end = e.end_time;
            }
        }
        return p;
    }

    [[nodiscard]] double evaluate(const TimeTableState& state,
                                  const TimeTableProblem& problem) const
    {
        return weight * penalty(state, problem);
    }

    [[nodiscard]] bool is_satisfied(const TimeTableState& state,
                                    const TimeTableProblem& /*problem*/) const
    {
        for (const auto& [key, entries] : lookup_)
        {
            int prev_end = -1;
            for (const auto& e : entries)
            {
                if (!state.is_attended(e.class_id, e.group)) continue;
                if (prev_end >= 0 && e.start_time - prev_end < min_break)
                    return false;
                prev_end = e.end_time;
            }
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

    // -------------------- PartiallyEvaluatable interface --------------------
    //
    // Each partial_* method restricts computation to the (day, week) buckets that
    // contain the given (class_id, group).  This is correct because:
    //
    //   partial_penalty <= full_penalty  (subset of buckets)
    //
    // For is_satisfied / is_feasible this means: if the constraint is violated in
    // any affected bucket it is detected immediately; violations in unaffected
    // buckets were already caught when those classes were assigned.
    //
    // For is_feasible: if even the partial penalty exceeds best+slack then the
    // full penalty certainly does too → safe to prune.  The reverse is not true,
    // so some prune opportunities are missed, but no valid state is ever rejected.

    [[nodiscard]] double partial_penalty(const TimeTableState& state,
                                         const TimeTableProblem& /*problem*/,
                                         const int class_id, const int group) const
    {
        const auto it = class_group_to_keys_.find({class_id, group});
        if (it == class_group_to_keys_.end()) return 0.0;

        double p = 0.0;
        for (const auto& key : it->second)
        {
            const auto& entries = lookup_.at(key);
            int prev_end = -1;
            for (const auto& e : entries)
            {
                if (!state.is_attended(e.class_id, e.group)) continue;
                if (prev_end >= 0)
                {
                    const int gap = e.start_time - prev_end;
                    if (gap > min_break)
                        p += static_cast<double>(gap - min_break);
                }
                prev_end = e.end_time;
            }
        }
        return p;
    }

    [[nodiscard]] double partial_evaluate(const TimeTableState& state,
                                          const TimeTableProblem& problem,
                                          const int class_id, const int group) const
    {
        return weight * partial_penalty(state, problem, class_id, group);
    }

    [[nodiscard]] bool partial_is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& /*problem*/,
                                             const int class_id, const int group) const
    {
        const auto it = class_group_to_keys_.find({class_id, group});
        if (it == class_group_to_keys_.end()) return true;

        for (const auto& key : it->second)
        {
            const auto& entries = lookup_.at(key);
            int prev_end = -1;
            for (const auto& e : entries)
            {
                if (!state.is_attended(e.class_id, e.group)) continue;
                if (prev_end >= 0 && e.start_time - prev_end < min_break)
                    return false;
                prev_end = e.end_time;
            }
        }
        return true;
    }

    [[nodiscard]] bool partial_is_feasible(const TimeTableState& state,
                                            const TimeTableProblem& problem,
                                            const SequenceContext& context,
                                            const int class_id, const int group) const
    {
        if (!context.has_score(id)) return true;
        return partial_penalty(state, problem, class_id, group) <= context[id] + slack;
    }

private:
    struct Entry
    {
        int start_time;
        int end_time;
        int class_id;
        int group;
    };

    // (day, week_bit) → entries sorted by start_time.
    std::map<std::pair<int,int>, std::vector<Entry>> lookup_;

    // (class_id, group) → list of (day, week_bit) keys that contain this entry.
    // Built once in precompute(); used by partial_* methods to restrict to
    // the affected buckets.
    std::map<std::pair<int,int>, std::vector<std::pair<int,int>>> class_group_to_keys_;
};

static_assert(policies::Evaluatable<IntTimePolicy>,
    "IntTimePolicy must satisfy policies::Evaluatable");

static_assert(policies::PartiallyEvaluatable<IntTimePolicy>,
    "IntTimePolicy must satisfy PartiallyEvaluatable");

static_assert(policies::Substitutable<IntTimePolicy>,
    "IntTimePolicy must satisfy policies::Substitutable");

static_assert(concepts::ConstraintEvaluator<PolicyEvaluator<true, IntTimePolicy>>,
    "PolicyEvaluator<IntTimePolicy> must satisfy the ConstraintEvaluator concept");

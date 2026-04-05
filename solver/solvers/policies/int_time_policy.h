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
struct IntTimePolicy
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int min_break{};
    int id = 0;

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

    // Can also be called manually after default construction.
    void precompute(const TimeTableProblem& problem)
    {
        lookup_.clear();
        for (int class_id = 0; class_id < static_cast<int>(problem.class_size()); ++class_id)
        {
            const int max_group = problem.get_max_group(class_id);
            for (int group = 1; group <= max_group; ++group)
            {
                const auto& cls = problem.get_group(class_id, group);
                const Entry e{ cls.start_time, cls.end_time, class_id, group };
                if (cls.week.test(0)) lookup_[{cls.day, 0}].push_back(e);
                if (cls.week.test(1)) lookup_[{cls.day, 1}].push_back(e);
            }
        }
        // Sort each bucket by start_time once — never sorted again at runtime.
        for (auto& [key, entries] : lookup_)
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
                                   const constraints::SequenceContext& context) const
    {
        if (!context.has_score(id)) return true;
        return penalty(state, problem) <= context.best_scores[id] + slack;
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
    // Covers all (class_id, group) combinations — penalty() filters by
    // state.is_attended() at runtime to skip non-attended entries.
    std::map<std::pair<int,int>, std::vector<Entry>> lookup_;
};

static_assert(solver_models::Evaluatable<IntTimePolicy>,
    "IntTimePolicy must satisfy solver_models::Evaluatable");

static_assert(Evaluatable<PolicyConstraintEvaluator<IntTimePolicy>>,
    "PolicyConstraintEvaluator<IntTimePolicy> must satisfy the Evaluatable concept");
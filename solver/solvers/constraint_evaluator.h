//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <constraints.h>
#include <data_models.h>
#include <time_table_problem.h>
#include <time_table_state.h>

#include <vector>

// ConstraintEvaluator<TimePolicy> is the single place that knows about both
// constraint logic and the time representation.
//
// has_conflict() runs in the hot path of backtracking — it uses the policy's
// encoded TimeType for a fast overlap check.
//
// score() is called on complete (or near-complete) states — it delegates to
// constraints::evaluate_all which works on int times and handles all soft
// and hard constraint types.
template<typename TimePolicy>
class ConstraintEvaluator
{
public:
    explicit ConstraintEvaluator(const TimeTableProblem& problem)
        : problem_(problem)
    {
    }

    // True if candidate_id would time-conflict with any class already in state.
    [[nodiscard]] bool has_conflict(const int class_id, const int group, const TimeTableState& state) const
    {
        const auto& current_class = problem_.get_group(class_id, group);
        const auto& assigned_classes = state.get_assigned_classes();
        const auto& groups = state.get_raw_groups();
        for (const int assigned_class_id : assigned_classes)
        {
            const int assigned_group = groups[assigned_class_id];
            if (class_id != assigned_class_id && overlaps(current_class, problem_.get_group(assigned_class_id, assigned_group)))
            {
                return true;
            }
        }
        return false;
    }

    // Total weighted penalty for the state (lower = better).
    [[nodiscard]] double score(const TimeTableState& state) const
    {
        return constraints::evaluate_all(problem_, state);
    }

private:
    const TimeTableProblem& problem_;

    [[nodiscard]] static bool overlaps(const solver_models::Class& class_a, const solver_models::Class& class_b)
    {
        if (class_a.day != class_b.day)
        {
            return false;
        }
        if (!(class_a.week & class_b.week).any())
        {
            return false;
        }
        if (class_a.end_time <= class_b.start_time
            || class_b.end_time <= class_a.start_time)
        {
            return false;
        }
        return true;
    }
};

class BaseEvaluator
{
public:
    explicit BaseEvaluator(const TimeTableProblem& problem)
        : problem_(problem) {}

    [[nodiscard]] constraints::SequenceContext score(const TimeTableState& state, int sequence) const;
    void update_context(constraints::SequenceContext& context, const TimeTableState& state, int sequence) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, int sequence) const;
    [[nodiscard]] bool   are_satisfied(const TimeTableState& state, int sequence) const;
    [[nodiscard]] bool   are_feasible(const TimeTableState& state, const constraints::SequenceContext& context, int sequence) const;

private:
    const TimeTableProblem& problem_;
};

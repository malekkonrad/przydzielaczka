//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <constraints.h>
#include <data_models.h>
#include <time_table_problem.h>
#include <time_table_state.h>

#include <unordered_map>
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
        const auto& classes = problem_.get_classes();
        const auto& groups = problem_.get_max_group();
        classes_.resize(groups.size());
        for (int i = 0; i < groups.size(); i++)
        {
            classes_[i].resize(groups[i] + 1);
        }
        for (const auto& clazz : classes)
        {
            classes_[clazz.id][clazz.group] = clazz;
        }
    }

    // True if candidate_id would time-conflict with any class already in state.
    [[nodiscard]] bool has_conflict(const int class_id, const int group, const TimeTableState& state) const
    {
        const auto& assigned_classes = state.get_assigned_classes();
        const auto& groups = state.get_groups();
        for (const int assigned_class_id : assigned_classes)
        {
            const int assigned_group = groups[assigned_class_id];
            if (class_id != assigned_class_id && overlaps(class_id, group, assigned_class_id, assigned_group))
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
    std::vector<std::vector<solver_models::Class>> classes_;

    [[nodiscard]] bool overlaps(const int class_id_a, const int group_a, const int class_id_b, const int group_b) const
    {
        const auto& class_a = classes_[class_id_a][group_a];
        const auto& class_b = classes_[class_id_b][group_b];
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

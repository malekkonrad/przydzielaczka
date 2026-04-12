//
// Created by mateu on 12.04.2026.
//

#pragma once

#include <utility>
#include <vector>
#include <iostream>
#include <time_table_state.h>
#include <time_table_problem.h>

// A lightweight range over (class_id, group) pairs.
//
// Two modes:
//
//   Simple  — constructed with (class_id, first_group, last_group).
//             Yields (class_id, first_group), …, (class_id, last_group).
//
//   Vector  — constructed with (order, state).
//             Iterates a pre-built std::vector<std::pair<int,int>> in order,
//             skipping entries whose class_id is already assigned in `state`.
//             begin() automatically resumes after the last committed pair so
//             that each recursive depth starts exactly one step past the pair
//             chosen at the previous depth.

class ClassGroupRange
{
public:

    explicit ClassGroupRange(const TimeTableProblem& problem)
        : problem_(problem)
    {
    };

    ClassGroupRange(const TimeTableProblem& problem, const std::vector<std::pair<int, int>>& order)
        : problem_(problem), order_(order)
    {
    };

    ClassGroupRange& operator=(const ClassGroupRange& other)
    {
        if (this != &other)
        {
            order_ = other.order_;
        }
        return *this;
    }

    [[nodiscard]] std::vector<std::pair<int, int>> get_class_groups(
        const TimeTableState& state,
        const int position) const
    {
        if (order_.empty())
        {
            return get_class_groups_simple(state);
        }
        else
        {
            return get_class_groups_order(state, position);
        }
    }

private:
    const TimeTableProblem& problem_;

    std::vector<std::pair<int,int>> order_;
    [[nodiscard]] std::vector<std::pair<int, int>> get_class_groups_simple(const TimeTableState& state) const
    {
        std::vector<std::pair<int,int>> class_groups;
        for (int class_id = 0; class_id < state.size(); ++class_id)
        {
            if (!state.is_assigned(class_id))
            {
                const int count = problem_.get_max_group(class_id);
                class_groups.reserve(count);
                for (int group = 1; group <= count; ++group)
                {
                    class_groups.emplace_back(class_id, group);
                }
                break;
            }
        }
        return class_groups;
    }

    [[nodiscard]] std::vector<std::pair<int, int>> get_class_groups_order(const TimeTableState& state, const int position) const
    {
        std::vector<std::pair<int,int>> class_groups;
        std::vector<int> running_count(state.size(), 0);

        for (int i = position; i < order_.size(); ++i)
        {
            const auto [class_id, _] = order_[i];
            running_count[class_id]++;
        }

        for (int i = position; i < order_.size(); ++i)
        {
            const auto class_group= order_[i];
            const auto class_id = class_group.first;
            if (state.is_assigned(class_id))
            {
                continue;
            }

            class_groups.emplace_back(class_group);
            // Check if other states are possible
            running_count[class_id]--;
            if (running_count[class_id] == 0)
            {
                break;
            }
        }
        return class_groups;
    }
};
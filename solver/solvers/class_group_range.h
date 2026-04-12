//
// Created by mateu on 12.04.2026.
//

#pragma once

#include <utility>
#include <vector>
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
        std::vector<std::pair<int,int>> class_groups;
        for (int class_id = 0; class_id < problem_.class_size(); ++class_id)
        {
            class_groups.emplace_back(class_id, problem_.get_max_group(class_id));
        }
        std::ranges::sort(class_groups, [](const std::pair<int,int>& a, const std::pair<int,int>& b){ return a.second < b.second; });
        simple_class_order_.reserve(class_groups.size());
        for (const auto& class_id : class_groups | std::views::keys)
        {
            simple_class_order_.push_back(class_id);
        }
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
            simple_class_order_ = other.simple_class_order_;
        }
        return *this;
    }

    [[nodiscard]] long long count_leaves(const TimeTableState& state, const int position) const
    {
        if (order_.empty())
        {
            return count_leaves_simple(state, position);
        }
        else
        {
            return count_leaves_order(state, position);
        }
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
    std::vector<int> simple_class_order_;

    [[nodiscard]] std::vector<std::pair<int, int>> get_class_groups_simple(const TimeTableState& state) const
    {
        std::vector<std::pair<int,int>> class_groups;
        // for (int class_id = 0; class_id < state.size(); ++class_id)
        for (const auto class_id : simple_class_order_)
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

    [[nodiscard]] long long count_leaves_simple(const TimeTableState& state, const int position) const
    {
        long long count = 1;
        const auto& groups = state.get_raw_groups();
        for (int i = 0; i < groups.size(); ++i)
        {
            const auto group = groups[i];
            if (group == TimeTableState::UNASSIGNED)
            {
                count *= problem_.get_max_group(i) * 2LL;
            }
        }
        return count;
    }

    [[nodiscard]] long long count_leaves_order(const TimeTableState& state, const int position) const
    {
        long long count = 1;
        std::vector<int> running_count(state.size(), 0);

        for (int i = position; i < order_.size(); ++i)
        {
            const auto [class_id, _] = order_[i];
            running_count[class_id]++;
        }

        for (int class_id = 0; class_id < running_count.size(); ++class_id)
        {
            const auto max_group = running_count[class_id];
            if (!state.is_assigned(class_id))
            {
                count *= max_group * 2LL;
            }
        }

        return count;
    }
};
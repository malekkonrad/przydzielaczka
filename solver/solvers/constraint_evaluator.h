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
        const auto& src = problem_.get_classes();
        classes_.reserve(src.size());
        for (std::size_t i = 0; i < src.size(); ++i)
        {
            const auto& c = src[i];
            classes_.push_back({
                c.id,
                c.lecturer,
                c.day,
                c.week,
                c.location,
                c.group,
                c.start_time,
                c.end_time,
                TimePolicy::encode(c.start_time, c.end_time)
            });
            id_to_index_[c.id] = static_cast<int>(i);
        }
    }

    // True if candidate_id would time-conflict with any class already in state.
    [[nodiscard]] bool has_conflict(int candidate_id, const TimeTableState& state) const
    {
        const InternalClass* candidate = find(candidate_id);
        if (!candidate) return false;

        // for (const int id : state.get_chosen_ids())
        // {
        //     const InternalClass* other = find(id);
        //     if (other && overlaps(*candidate, *other))
        //         return true;
        // }
        return false;
    }

    // Total weighted penalty for the state (lower = better).
    [[nodiscard]] double score(const TimeTableState& state) const
    {
        return constraints::evaluate_all(problem_, state);
    }

private:
    struct InternalClass
    {
        int id;
        int lecturer;
        int day;
        std::bitset<2> week;
        int location;
        int group;
        int start_min;  // original int times kept for gap/edge scoring
        int end_min;
        typename TimePolicy::TimeType time;  // policy-encoded interval
    };

    const TimeTableProblem&            problem_;
    std::vector<InternalClass>         classes_;
    std::unordered_map<int, int>       id_to_index_;  // class id → index in classes_

    [[nodiscard]] const InternalClass* find(int id) const
    {
        const auto it = id_to_index_.find(id);
        if (it == id_to_index_.end()) return nullptr;
        return &classes_[it->second];
    }

    [[nodiscard]] static bool overlaps(const InternalClass& a, const InternalClass& b)
    {
        if (a.day != b.day)           return false;
        if ((a.week & b.week).none()) return false;
        return TimePolicy::overlaps_time(a.time, b.time);
    }
};

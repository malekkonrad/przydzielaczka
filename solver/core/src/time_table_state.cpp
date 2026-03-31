//
// Created by mateu on 31.03.2026.
//

#include "time_table_state.h"

TimeTableState::TimeTableState(std::vector<int> chosen_ids)
    : chosen_ids_(chosen_ids.begin(), chosen_ids.end()) {}

void TimeTableState::add(const int class_id)
{
    chosen_ids_.insert(class_id);
}

void TimeTableState::remove(const int class_id)
{
    chosen_ids_.erase(class_id);
}

bool TimeTableState::contains(const int class_id) const
{
    return chosen_ids_.contains(class_id);
}

const std::unordered_set<int>& TimeTableState::get_chosen_ids() const
{
    return chosen_ids_;
}

int TimeTableState::size() const
{
    return static_cast<int>(chosen_ids_.size());
}

bool TimeTableState::is_empty() const
{
    return chosen_ids_.empty();
}

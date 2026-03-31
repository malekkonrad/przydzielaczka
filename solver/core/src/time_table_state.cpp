//
// Created by mateu on 31.03.2026.
//

#include "time_table_state.h"

#include <algorithm>
#include <ostream>
#include <vector>

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

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableState& s)
{
    // Print IDs in sorted order for readability.
    std::vector<int> ids(s.chosen_ids_.begin(), s.chosen_ids_.end());
    std::sort(ids.begin(), ids.end());

    out << "TimeTableState{ size=" << s.chosen_ids_.size() << ", chosen=[";
    for (std::size_t i = 0; i < ids.size(); ++i)
    {
        if (i > 0) out << ", ";
        out << ids[i];
    }
    out << "] }";
    return out;
}

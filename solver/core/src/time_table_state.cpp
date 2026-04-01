//
// Created by mateu on 31.03.2026.
//

#include <time_table_state.h>

#include <algorithm>
#include <ostream>
#include <vector>

TimeTableState::TimeTableState(const std::unordered_map<int, int>& groups)
    : groups_(groups)
{
}

TimeTableState::TimeTableState(const TimeTableState&& other) noexcept
    : groups_(std::move(other.groups_))
{
}

TimeTableState& TimeTableState::operator=(const TimeTableState&& other) noexcept
{
    if (this != &other)
    {
        groups_ = std::move(other.groups_);
    }
    return *this;
}

void TimeTableState::add(const int class_id, const int group)
{
    groups_[class_id] = group;
}

void TimeTableState::remove(const int class_id)
{
    groups_.erase(class_id);
}

void TimeTableState::remove(const int class_id, const int group)
{
    const auto found = groups_.find(class_id);
    if (found != groups_.end())
    {
        groups_.erase(found->first);
    }
}

bool TimeTableState::contains(const int class_id) const
{
    return groups_.contains(class_id);
}

bool TimeTableState::contains(const int class_id, const int group) const
{
    const auto found = groups_.find(class_id);
    if (found == groups_.end())
    {
        return false;
    }
    return found->second == group;
}

const std::unordered_map<int, int>& TimeTableState::get_groups() const
{
    return groups_;
}

size_t TimeTableState::size() const
{
    return groups_.size();
}

bool TimeTableState::is_empty() const
{
    return groups_.empty();
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableState& s)
{
    // Print IDs in sorted order for readability.
    std::vector<std::pair<int, int>> ids(s.groups_.begin(), s.groups_.end());
    std::ranges::sort(ids, [](auto& left, auto& right){return left.first < right.first;});

    out << "TimeTableState{ size=" << s.groups_.size() << ", chosen=[";
    for (std::size_t i = 0; i < ids.size(); i++)
    {
        if (i > 0) out << ", ";
        out << ids[i].first << ":" << ids[i].second;
    }
    out << "] }";
    return out;
}

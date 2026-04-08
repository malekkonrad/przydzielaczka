//
// Created by mateu on 31.03.2026.
//

#include <time_table_state.h>

#include <algorithm>
#include <ostream>
#include <vector>

// -------------------- CONSTRUCTORS --------------------

TimeTableState::TimeTableState(const size_t size)
    : groups_(size, UNASSIGNED)
{
}

TimeTableState::TimeTableState(std::vector<int> groups)
    : groups_(std::move(groups))
{
}

TimeTableState::TimeTableState(TimeTableState&& other) noexcept
    : groups_(std::move(other.groups_))
{
}

TimeTableState& TimeTableState::operator=(TimeTableState&& other) noexcept
{
    if (this != &other)
    {
        groups_ = std::move(other.groups_);
    }
    return *this;
}

// -------------------- MUTATORS --------------------

void TimeTableState::assign(const int class_id, const int group)
{
    groups_[static_cast<size_t>(class_id)] = -1 * std::abs(group);
}

void TimeTableState::attend(const int class_id, const int group)
{
    groups_[static_cast<size_t>(class_id)] = std::abs(group);
}

void TimeTableState::unassign(const int class_id)
{
    groups_[static_cast<size_t>(class_id)] = UNASSIGNED;
}

void TimeTableState::set_group(const int class_id, const int group)
{
    groups_[static_cast<size_t>(class_id)] = group;
}

void TimeTableState::reset_group(const int class_id)
{
    groups_[static_cast<size_t>(class_id)] = UNASSIGNED;
}

// -------------------- ACCESSORS --------------------

bool TimeTableState::is_assigned(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] != UNASSIGNED;
}

bool TimeTableState::is_assigned(const int class_id, const int group) const
{
    return std::abs(groups_[static_cast<size_t>(class_id)]) == std::abs(group);
}

bool TimeTableState::is_attended(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] > 0;
}

bool TimeTableState::is_attended(const int class_id, const int group) const
{
    return groups_[static_cast<size_t>(class_id)] == std::abs(group);
}

bool TimeTableState::is_unattended(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] < 0;
}

bool TimeTableState::is_unattended(const int class_id, const int group) const
{
    return groups_[static_cast<size_t>(class_id)] == -1 * std::abs(group);
}

bool TimeTableState::is_group(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] != UNASSIGNED;
}

bool TimeTableState::is_group(const int class_id, const int group) const
{
    return groups_[static_cast<size_t>(class_id)] == group;
}

const std::vector<int>& TimeTableState::get_raw_groups() const
{
    return groups_;
}

std::vector<int> TimeTableState::get_assigned_groups() const
{
    std::vector<int> groups = groups_; // explicit copy
    std::ranges::for_each(groups, [](int& g) { g = std::abs(g); });
    return groups;
}

int TimeTableState::get_raw_group(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)];
}

int TimeTableState::get_assigned_group(const int class_id) const
{
    return std::abs(groups_[static_cast<size_t>(class_id)]);
}

size_t TimeTableState::size() const
{
    return groups_.size();
}

size_t TimeTableState::filled() const
{
    return static_cast<size_t>(std::ranges::count_if(groups_, [](const int g) { return g != UNASSIGNED; }));
}

size_t TimeTableState::assigned() const
{
    return filled();
}

size_t TimeTableState::attended() const
{
    return static_cast<size_t>(std::ranges::count_if(groups_, [](const int g) { return g > 0; }));
}

size_t TimeTableState::unattended() const
{
    return static_cast<size_t>(std::ranges::count_if(groups_, [](const int g) { return g < 0; }));
}

bool TimeTableState::is_empty() const
{
    return filled() == 0;
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableState& s)
{
    const auto& groups = s.get_raw_groups();
    out << "TimeTableState{ size=" << groups.size() << ", filled=[";
    bool first = true;
    for (size_t i = 0; i < groups.size(); ++i)
    {
        if (groups[i] != TimeTableState::UNASSIGNED)
        {
            if (!first) out << ", ";
            out << i << ":" << groups[i];
            first = false;
        }
    }
    out << "] }";
    return out;
}
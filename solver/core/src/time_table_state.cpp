//
// Created by mateu on 31.03.2026.
//

#include <time_table_state.h>

#include <algorithm>
#include <ostream>
#include <vector>

// -------------------- CONSTRUCTORS --------------------

TimeTableState::TimeTableState(const size_t size)
    : groups_(size, -1)
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
    groups_[static_cast<size_t>(class_id)] = group;
}

void TimeTableState::unassign(const int class_id)
{
    groups_[static_cast<size_t>(class_id)] = -1;
}

// -------------------- ACCESSORS --------------------

bool TimeTableState::is_assigned(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] >= 0;
}

bool TimeTableState::is_assigned(const int class_id, const int group) const
{
    return groups_[static_cast<size_t>(class_id)] == group;
}

const std::vector<int>& TimeTableState::get_groups() const
{
    return groups_;
}

int TimeTableState::get_group(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)];
}

const std::vector<int>& TimeTableState::get_assigned_classes() const
{
    thread_local std::vector<int> classes;
    classes.clear();

    const int groups_size = static_cast<int>(groups_.size());
    for (int class_id = 0; class_id < groups_size; class_id++)
    {
        if (is_assigned(class_id))
        {
            classes.emplace_back(class_id);
        }
    }
    return classes;
}

size_t TimeTableState::size() const
{
    return groups_.size();
}

size_t TimeTableState::filled() const
{
    return static_cast<size_t>(
        std::count_if(groups_.begin(), groups_.end(), [](int g) { return g >= 0; }));
}

bool TimeTableState::is_empty() const
{
    return filled() == 0;
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableState& s)
{
    const auto& groups = s.get_groups();
    out << "TimeTableState{ size=" << groups.size() << ", filled=[";
    bool first = true;
    for (size_t i = 0; i < groups.size(); ++i)
    {
        if (groups[i] >= 0)
        {
            if (!first) out << ", ";
            out << i << ":" << groups[i];
            first = false;
        }
    }
    out << "] }";
    return out;
}
//
// Created by mateu on 31.03.2026.
//

#include <time_table_state.h>

#include <algorithm>
#include <ostream>

// -------------------- CONSTRUCTORS --------------------

template<size_t ClassSize>
TimeTableState<ClassSize>::TimeTableState(std::array<int, ClassSize> groups)
    : groups_(std::move(groups))
{
}

template<size_t ClassSize>
TimeTableState<ClassSize>::TimeTableState(const TimeTableState&& other) noexcept
    : groups_(std::move(other.groups_))
{
}

template<size_t ClassSize>
TimeTableState<ClassSize>& TimeTableState<ClassSize>::operator=(TimeTableState&& other) noexcept
{
    if (this != &other)
    {
        groups_ = std::move(other.groups_);
    }
    return *this;
}

// -------------------- MUTATORS --------------------

template<size_t ClassSize>
void TimeTableState<ClassSize>::assign(const int class_id, const int group)
{
    groups_[static_cast<size_t>(class_id)] = group;
}

template<size_t ClassSize>
void TimeTableState<ClassSize>::unassign(const int class_id)
{
    groups_[static_cast<size_t>(class_id)] = -1;
}

// -------------------- ACCESSORS --------------------

template<size_t ClassSize>
bool TimeTableState<ClassSize>::is_assigned(const int class_id) const
{
    return groups_[static_cast<size_t>(class_id)] >= 0;
}

template<size_t ClassSize>
bool TimeTableState<ClassSize>::is_assigned(const int class_id, const int group) const
{
    return groups_[static_cast<size_t>(class_id)] == group;
}

template<size_t ClassSize>
const std::array<int, ClassSize>& TimeTableState<ClassSize>::get_groups() const
{
    return groups_;
}

template<size_t ClassSize>
size_t TimeTableState<ClassSize>::size() const
{
    return ClassSize;
}

template<size_t ClassSize>
size_t TimeTableState<ClassSize>::filled() const
{
    return static_cast<size_t>(
        std::count_if(groups_.begin(), groups_.end(), [](int g) { return g >= 0; }));
}

template<size_t ClassSize>
bool TimeTableState<ClassSize>::is_empty() const
{
    return filled() == 0;
}

// -------------------- STREAM --------------------

// Uses get_groups() (public) so no private access is required.
template<size_t ClassSize>
std::ostream& operator<<(std::ostream& out, const TimeTableState<ClassSize>& s)
{
    out << "TimeTableState{ size=" << ClassSize << ", filled=[";
    const auto& groups = s.get_groups();
    bool first = true;
    for (size_t i = 0; i < ClassSize; ++i)
    {
        if (groups[i] != -1)
        {
            if (!first) out << ", ";
            out << i << ":" << groups[i];
            first = false;
        }
    }
    out << "] }";
    return out;
}

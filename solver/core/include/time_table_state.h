//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <iosfwd>

template<size_t ClassSize>
class TimeTableState
{
public:
    TimeTableState() = default;
    explicit TimeTableState(std::array<int, ClassSize> groups);

    TimeTableState(const TimeTableState& other) = default;
    TimeTableState& operator=(const TimeTableState& other) = default;
    TimeTableState(const TimeTableState&& other) noexcept;
    TimeTableState& operator=(TimeTableState&& other) noexcept;
    ~TimeTableState() = default;

    void assign(int class_id, int group);
    void unassign(int class_id);

    [[nodiscard]] bool is_assigned(int class_id) const;
    [[nodiscard]] bool is_assigned(int class_id, int group) const;
    [[nodiscard]] const std::array<int, ClassSize>& get_groups() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t filled() const;
    [[nodiscard]] bool is_empty() const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableState& s);

private:
    std::array<int, ClassSize> groups_;
};

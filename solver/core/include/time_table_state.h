//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <iosfwd>

class TimeTableState
{
public:
    TimeTableState() = default;
    explicit TimeTableState(const std::unordered_map<int, int>& groups);

    TimeTableState(const TimeTableState& other) = default;
    TimeTableState& operator=(const TimeTableState& other) = default;
    TimeTableState(const TimeTableState&& other) noexcept;
    TimeTableState& operator=(const TimeTableState&& other) noexcept;
    ~TimeTableState() = default;

    void add(int class_id, int group);
    void remove(int class_id);
    void remove(int class_id, int group);

    [[nodiscard]] bool contains(int class_id) const;
    [[nodiscard]] bool contains(int class_id, int group) const;
    [[nodiscard]] const std::unordered_map<int, int>& get_groups() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool is_empty() const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableState& s);

private:
    std::unordered_map<int, int> groups_;
};

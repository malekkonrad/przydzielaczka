//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <ranges>
#include <vector>

// Represents a partial or complete assignment of students to class groups.
//
// Each class_id maps to a single integer stored in groups_[class_id]:
//
//   UNASSIGNED (0)   — the class has not been assigned any group yet.
//   Positive  (+g)   — the student is ATTENDING group g (physically present).
//   Negative  (-g)   — the student is ASSIGNED to group g but NOT ATTENDING
//                      (registered in the group, but skipping the class).
//
// Group IDs are 1-indexed. Zero is reserved for UNASSIGNED and must never be
// used as a real group identifier.
//
// Terminology used throughout the codebase:
//   "assigned"   — the class has any non-zero value (attending or not).
//   "attended"   — the class has a positive value (physically present).
//   "unattended" — the class has a negative value (registered but absent).
//   "unassigned" — the class has value 0 (no group chosen at all).
class TimeTableState
{
public:
    // Sentinel value meaning "no group chosen for this class".
    static constexpr int UNASSIGNED = 0;

    explicit TimeTableState(size_t size);
    explicit TimeTableState(std::vector<int> groups);

    TimeTableState(const TimeTableState& other) = default;
    TimeTableState& operator=(const TimeTableState& other) = default;
    TimeTableState(TimeTableState&& other) noexcept;
    TimeTableState& operator=(TimeTableState&& other) noexcept;
    ~TimeTableState() = default;

    // Stores -group: the student is registered in this group but will not attend.
    void assign(int class_id, int group);
    // Stores +group: the student is physically attending this group.
    void attend(int class_id, int group);
    // Stores UNASSIGNED (0): removes any group choice for this class.
    void unassign(int class_id);

    // Stores the raw value directly — use assign/attend/unassign instead.
    void set_group(int class_id, int group);
    // Alias for unassign.
    void reset_group(int class_id);

    // True if the class has any group (attending or not), i.e. groups_[class_id] != 0.
    [[nodiscard]] bool is_assigned(int class_id) const;
    // True if the class is assigned to the given group (attending or not).
    [[nodiscard]] bool is_assigned(int class_id, int group) const;

    // True if the student is physically attending this class, i.e. groups_[class_id] > 0.
    [[nodiscard]] bool is_attended(int class_id) const;
    // True if the student is physically attending the given group of this class.
    [[nodiscard]] bool is_attended(int class_id, int group) const;

    // True if the student is registered but NOT attending, i.e. groups_[class_id] < 0.
    [[nodiscard]] bool is_unattended(int class_id) const;
    // True if the student is registered in the given group but not attending.
    [[nodiscard]] bool is_unattended(int class_id, int group) const;

    // Alias for is_assigned(class_id) — true when any group is set.
    [[nodiscard]] bool is_group(int class_id) const;
    // True if groups_[class_id] == group (exact raw match, sign included).
    [[nodiscard]] bool is_group(int class_id, int group) const;

    // Raw storage: positive = attending, negative = not attending, 0 = unassigned.
    [[nodiscard]] const std::vector<int>& get_raw_groups() const;
    // Returns a copy with all values replaced by their absolute value (group IDs only).
    [[nodiscard]] std::vector<int> get_assigned_groups() const;
    // Returns the raw stored value for class_id (may be negative or 0).
    // For attending classes this equals the group ID directly.
    // For not-attending classes use std::abs() to get the group ID.
    [[nodiscard]] int get_raw_group(int class_id) const;
    // Returns the assigned group for class_id (may be positive or 0 - UNASSIGNED).
    [[nodiscard]] int get_assigned_group(int class_id) const;

    // Lazy ranges over class IDs — nothing is allocated, evaluated on demand.
    // Safe to use in range-for or pass to std::ranges algorithms.
    // Lifetime is tied to this TimeTableState — do not outlive it.

    // Yields class_ids
    [[nodiscard]] auto get_classes() const
    {
        return std::views::iota(0, static_cast<int>(groups_.size()));
    }

    // Yields class_ids where groups_[id] != UNASSIGNED (attending or not).
    [[nodiscard]] auto get_assigned_classes() const
    {
        return std::views::iota(0, static_cast<int>(groups_.size()))
             | std::views::filter([this](const int id) { return groups_[id] != UNASSIGNED; });
    }

    // Yields class_ids where groups_[id] > 0 (attending).
    [[nodiscard]] auto get_attended_classes() const
    {
        return std::views::iota(0, static_cast<int>(groups_.size()))
             | std::views::filter([this](const int id) { return groups_[id] > 0; });
    }

    // Yields class_ids where groups_[id] < 0 (assigned but not attending).
    [[nodiscard]] auto get_unattended_classes() const
    {
        return std::views::iota(0, static_cast<int>(groups_.size()))
             | std::views::filter([this](const int id) { return groups_[id] < 0; });
    }

    // Number of class slots (including unassigned ones).
    [[nodiscard]] size_t size() const;
    // Number of classes that have any group set (attending or not).
    [[nodiscard]] size_t filled() const;
    // Alias for filled().
    [[nodiscard]] size_t assigned() const;
    // Number of classes that have any group set to positive (attending).
    [[nodiscard]] size_t attended() const;
    // Number of classes that have any group set to negative (not attending).
    [[nodiscard]] size_t unattended() const;

    [[nodiscard]] bool is_empty() const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableState& s);

private:
    std::vector<int> groups_;
};
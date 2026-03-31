//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <iosfwd>
#include <unordered_set>
#include <vector>

class TimeTableState
{
public:
    TimeTableState() = default;
    explicit TimeTableState(std::vector<int> chosen_ids);

    void add(int class_id);
    void remove(int class_id);

    [[nodiscard]] bool contains(int class_id) const;
    [[nodiscard]] const std::unordered_set<int>& get_chosen_ids() const;
    [[nodiscard]] int size() const;
    [[nodiscard]] bool is_empty() const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableState& s);

private:
    std::unordered_set<int> chosen_ids_;
};

//
// Created by mateu on 30.03.2026.
//

#pragma once

#include "constraints.h"
#include "data_models.h"

#include <iosfwd>
#include <span>
#include <vector>

class TimeTableProblem
{
public:
    explicit TimeTableProblem(std::vector<solver_models::Class> classes,
                              std::vector<solver_models::ConstraintVariant> constraints);

    TimeTableProblem(const TimeTableProblem& other) = default;
    TimeTableProblem& operator=(const TimeTableProblem& other) = default;
    TimeTableProblem(TimeTableProblem&& other) noexcept;
    TimeTableProblem& operator=(TimeTableProblem&& other) noexcept;

    [[nodiscard]] size_t size() const;
    [[nodiscard]] size_t sequence_size() const;
    [[nodiscard]] size_t class_size() const;
    [[nodiscard]] const std::vector<int>& get_max_group() const;
    [[nodiscard]] int get_max_group(int class_id) const;
    [[nodiscard]] const std::vector<std::vector<solver_models::Class>>& get_classes() const;
    [[nodiscard]] const std::vector<solver_models::Class>& get_groups(int class_id) const;
    [[nodiscard]] const solver_models::Class& get_class(int class_id, int group) const;
    [[nodiscard]] const std::vector<solver_models::ConstraintVariant>& get_constraints() const;
    [[nodiscard]] std::span<const solver_models::ConstraintVariant> get_constraints(int sequence) const;
    [[nodiscard]] std::span<const solver_models::ConstraintVariant> get_goals(int sequence) const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p);

private:
    std::vector<int> max_group_;
    std::vector<std::vector<solver_models::Class>> classes_;
    std::vector<solver_models::ConstraintVariant> constraints_;
    std::vector<int> sequence_soft_hard_split_point_;
    std::vector<int> sequence_split_point_;
};

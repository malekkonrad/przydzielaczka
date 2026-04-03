//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <data_models.h>

#include <iosfwd>
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
    [[nodiscard]] const std::vector<solver_models::Class>& get_classes() const;
    [[nodiscard]] const std::vector<solver_models::ConstraintVariant>& get_constraints() const;
    [[nodiscard]] const std::vector<solver_models::ConstraintVariant>& get_constraints(int sequence) const;
    [[nodiscard]] const std::vector<solver_models::ConstraintVariant>& get_goals(int sequence) const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p);

private:
    std::vector<solver_models::Class> classes_;
    std::vector<solver_models::ConstraintVariant> constraints_;
    std::vector<int> sequence_split_point_;
};

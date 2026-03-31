//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <iosfwd>
#include <vector>

#include "solver_data_models.h"

class TimeTableProblem
{
public:
    TimeTableProblem() = default;
    explicit TimeTableProblem(std::vector<solver_models::Class> classes, std::vector<solver_models::ConstraintVariant> constraints);

    [[nodiscard]] const std::vector<solver_models::Class>& get_classes() const;
    [[nodiscard]] const std::vector<solver_models::ConstraintVariant>& get_constraints() const;

    [[nodiscard]] const solver_models::Class* find_class(int id) const;
    [[nodiscard]] std::vector<const solver_models::ConstraintVariant*> get_constraints_for_class(int class_id) const;

    [[nodiscard]] bool is_valid() const;

    friend std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p);

private:
    std::vector<solver_models::Class> classes_;
    std::vector<solver_models::ConstraintVariant> constraints_;
};

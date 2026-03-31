//
// Created by mateu on 30.03.2026.
//

#include "time_table_problem.h"

#include <algorithm>
#include <ostream>
#include <optional>
#include <variant>

using namespace solver_models;

// Returns the class_id carried by a constraint variant, or nullopt if the
// constraint type is not tied to a specific class.
static std::optional<int> constraint_class_id(const ConstraintVariant& v)
{
    return std::visit([](const auto& c) -> std::optional<int>
    {
        using T = std::decay_t<decltype(c)>;
        if constexpr (
            std::is_same_v<T, GroupPreferenceConstraint>       ||
            std::is_same_v<T, LecturerPreferenceConstraint>    ||
            std::is_same_v<T, MaximizeSingleAttendanceConstraint> ||
            std::is_same_v<T, PreferEdgeClassesConstraint>)
        {
            return c.class_id;
        }
        return std::nullopt;
    }, v);
}

// -------------------- CONSTRUCTORS --------------------

TimeTableProblem::TimeTableProblem(
    std::vector<Class> classes,
    std::vector<ConstraintVariant> constraints)
    : classes_(std::move(classes))
    , constraints_(std::move(constraints)) {}

// -------------------- ACCESSORS --------------------

const std::vector<Class>& TimeTableProblem::get_classes() const
{
    return classes_;
}

const std::vector<ConstraintVariant>& TimeTableProblem::get_constraints() const
{
    return constraints_;
}

// -------------------- QUERIES --------------------

const Class* TimeTableProblem::find_class(const int id) const
{
    const auto it = std::ranges::find_if(classes_,
        [id](const Class& c) { return c.id == id; });

    return it != classes_.end() ? &(*it) : nullptr;
}

std::vector<const ConstraintVariant*> TimeTableProblem::get_constraints_for_class(const int class_id) const
{
    std::vector<const ConstraintVariant*> result;

    for (const auto& v : constraints_)
    {
        const auto id = constraint_class_id(v);
        if (id.has_value() && *id == class_id)
            result.push_back(&v);
    }

    return result;
}

bool TimeTableProblem::is_valid() const
{
    for (const auto& c : classes_)
    {
        if (c.start_time >= c.end_time)
            return false;
    }

    for (const auto& v : constraints_)
    {
        const auto id = constraint_class_id(v);
        if (id.has_value() && find_class(*id) == nullptr)
            return false;
    }

    return true;
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p)
{
    out << "TimeTableProblem{ classes=" << p.classes_.size()
        << ", constraints=" << p.constraints_.size() << " }";
    return out;
}

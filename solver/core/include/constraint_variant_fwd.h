//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <constraints.h>

#include <variant>

// -------------------- VARIANT --------------------

namespace solver_models
{
    struct MaximizeTotalAttendanceConstraint;
    struct MaximizeGroupAttendanceConstraint;
    struct MaximizeClassAttendanceConstraint;
    struct PreferEdgeGroupConstraint;
    struct PreferEdgeClassConstraint;
    struct TimeBlockDateConstraint;
    struct TimeBlockDayConstraint;
    struct GroupPreferenceConstraint;
    struct LecturerPreferenceConstraint;
    struct MinimizeGapsConstraint;

    using ConstraintVariant = std::variant<
        MinimizeGapsConstraint,
        GroupPreferenceConstraint,
        LecturerPreferenceConstraint,
        TimeBlockDayConstraint,
        TimeBlockDateConstraint,
        PreferEdgeClassConstraint,
        PreferEdgeGroupConstraint,
        MaximizeClassAttendanceConstraint,
        MaximizeGroupAttendanceConstraint,
        MaximizeTotalAttendanceConstraint
    >;

} // namespace solver_models

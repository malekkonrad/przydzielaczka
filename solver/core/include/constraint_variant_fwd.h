//
// Created by mateu on 04.04.2026.
//

#pragma once

// Forward declarations only — ConstraintVariant is defined in constraints.h
// after all struct bodies are complete.

namespace solver_models
{
    struct MinimizeTotalAbsenceConstraint;
    struct MinimizeGroupAbsenceConstraint;
    struct MinimizeClassAbsenceConstraint;
    struct PreferEdgeGroupConstraint;
    struct PreferEdgeClassConstraint;
    struct TimeBlockDateConstraint;
    struct TimeBlockDayConstraint;
    struct GroupPreferenceConstraint;
    struct LecturerPreferenceConstraint;
    struct MinimizeGapsConstraint;
} // namespace solver_models

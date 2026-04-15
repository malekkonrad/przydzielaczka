//
// Created by mateu on 15.04.2026.
//

#pragma once

#include <constraints.h>
#include <solver_config.h>
#include <traits.h>

// No-op policy — always satisfied, zero penalty.
template<SolverTraitsConcept Traits>
struct NullPolicy
{
    int id       = -1;
    int sequence = -1;
    bool hard    = false;
    constraints::ConstraintType type = constraints::ConstraintType::Null;

    static NullPolicy make(const solver_models::ConstraintVariant&, const TimeTableProblem&) { return {}; }

    [[nodiscard]] double penalty(const TimeTableState&, const TimeTableProblem&) const { return 0.0; }
    [[nodiscard]] double evaluate(const TimeTableState&, const TimeTableProblem&) const { return 0.0; }
    [[nodiscard]] bool   is_satisfied(const TimeTableState&, const TimeTableProblem&) const { return true; }
    [[nodiscard]] bool   is_feasible(const TimeTableState&, const TimeTableProblem&, const SequenceContext&) const { return true; }
};

static_assert(policies::Evaluatable<NullPolicy<SolverTraits>>,
    "NullPolicy must satisfy policies::Evaluatable");

static_assert(policies::Substitutable<NullPolicy<SolverTraits>, SolverTraits>,
    "NullPolicy must satisfy policies::Substitutable");
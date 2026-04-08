//
// Created by mateu on 08.04.2026.
//

#pragma once

#include <constraint_evaluator.h>
#include <policies/policies.h>

// BasicSolverTraits — fluent compile-time builder for solver configuration.
//
// Start from SolverTraits (the default alias) and chain named modifiers:
//
//   SolverTraits
//       ::WithPartial<true>
//       ::WithPolicies<IntTimePolicy, IntAbsencePolicy>
//
// Order of chaining does not matter — each modifier carries all accumulated
// settings forward, leaving the rest at their defaults.  The resulting type
// is passed directly as the template argument to any solver.
//
// Defaults: use_partial = true, no policies.

template<bool UsePartial = true, policies::Substitutable... Ps>
struct BasicSolverTraits
{
    static constexpr bool use_partial = UsePartial;
    using Evaluator = PolicyEvaluator<UsePartial, Ps...>;

    // Enable / disable per-(class_id, group) partial evaluation.
    template<bool V>
    using WithPartial = BasicSolverTraits<V, Ps...>;

    // Replace the active policy list entirely.
    template<policies::Substitutable... NewPs>
    using WithPolicies = BasicSolverTraits<UsePartial, NewPs...>;
};

// Entry point — begin a chain from here.
using SolverTraits = BasicSolverTraits<>;

// Convenience alias for the no-policy, no-partial case that uses BaseEvaluator.
struct DefaultSolverTraits
{
    static constexpr bool use_partial = false;
    using Evaluator = BaseEvaluator;
};
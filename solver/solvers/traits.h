//
// Created by mateu on 08.04.2026.
//

#pragma once

#include <solver_config.h>
#include <policies/policies.h>

// BasicSolverTraits — fluent compile-time builder for solver configuration.
//
// Start from SolverTraits (the default alias) and chain named modifiers:
//
//   SolverTraits
//       ::WithPartialEvaluation<false>
//       ::WithBranchAndBound<true>
//       ::WithPolicies<IntTimePolicy, IntAbsencePolicy>
//
// Ps... are template template parameters (uninstantiated policy templates).
// BasicConstraintEvaluator instantiates them as Ps<Traits> internally, so
// there is no self-referential type at the call site.

template<detail::SolverConfig Config = {}, template<typename> class... Ps>
struct BasicSolverTraits {
    static constexpr detail::SolverConfig config = Config;

    template<bool V> using WithPartialEvaluation     = BasicSolverTraits<Config.with_partial_evaluation(V),     Ps...>;
    template<bool V> using WithSimplifiedEvaluation  = BasicSolverTraits<Config.with_simplified_evaluation(V), Ps...>;
    template<bool V> using WithHeuristic             = BasicSolverTraits<Config.with_heuristic(V),              Ps...>;
    template<bool V> using WithMRV                   = BasicSolverTraits<Config.with_mrv(V),                    Ps...>;
    template<bool V> using WithForwardChecking       = BasicSolverTraits<Config.with_forward_checking(V),       Ps...>;
    template<bool V> using WithConstraintPropagation = BasicSolverTraits<Config.with_constraint_propagation(V), Ps...>;
    template<bool V> using WithBranchAndBound        = BasicSolverTraits<Config.with_branch_and_bound(V),       Ps...>;
    template<bool V> using WithCache                 = BasicSolverTraits<Config.with_cache(V),                  Ps...>;
    template<bool V> using WithPrecomputedConflicts  = BasicSolverTraits<Config.with_precomputed_conflicts(V),  Ps...>;
    template<bool V> using WithDominancePruning      = BasicSolverTraits<Config.with_dominance_pruning(V),      Ps...>;
    template<bool V> using WithSequenceSquashing     = BasicSolverTraits<Config.with_sequence_squashing(V),     Ps...>;
    template<bool V> using WithMeetInTheMiddle       = BasicSolverTraits<Config.with_meet_in_the_middle(V),     Ps...>;
    template<bool V> using WithIncrementalScoring    = BasicSolverTraits<Config.with_incremental_scoring(V),    Ps...>;
    template<bool V> using WithMultiGoalEvaluation   = BasicSolverTraits<Config.with_multi_goal_evaluation(V),  Ps...>;

    // Replace the active policy list entirely.
    template<template<typename> class... NewPs>
    using WithPolicies = BasicSolverTraits<Config, NewPs...>;

    // Append policies to the existing list.
    template<template<typename> class... NewPs>
    using AddPolicies = BasicSolverTraits<Config, Ps..., NewPs...>;
};

// Entry point — begin a chain from here.
using SolverTraits = BasicSolverTraits<>;
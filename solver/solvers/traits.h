//
// Created by mateu on 08.04.2026.
//

#pragma once

#include "policies/policies.h"

namespace detail {

// SolverConfig — plain structural aggregate used as a C++20 NTTP.
//
// Because it is a structural type (all public, no user-provided copy ctor),
// it can be a non-type template parameter.  Every field has a constexpr
// wither that returns a modified copy, letting BasicSolverTraits expose
// fluent With… aliases without repeating every parameter by hand.
struct SolverConfig {
    bool use_partial_evaluation     = false;
    bool use_heuristic              = false;
    bool use_mrv                    = false;
    bool use_forward_checking       = false;
    bool use_constraint_propagation = false;
    bool use_branch_and_bound       = false;
    bool use_cache                  = false;
    bool use_precomputed_conflicts  = false;
    bool use_dominance_pruning      = false;
    bool use_sequence_squashing     = false;
    bool use_meet_in_the_middle     = false;
    bool use_incremental_scoring    = false;

    [[nodiscard]] constexpr SolverConfig with_partial_evaluation(const bool v) const     { auto c=*this; c.use_partial_evaluation=v;     return c; }
    [[nodiscard]] constexpr SolverConfig with_heuristic(const bool v) const              { auto c=*this; c.use_heuristic=v;              return c; }
    [[nodiscard]] constexpr SolverConfig with_mrv(const bool v) const                    { auto c=*this; c.use_mrv=v;                    return c; }
    [[nodiscard]] constexpr SolverConfig with_forward_checking(const bool v) const       { auto c=*this; c.use_forward_checking=v;       return c; }
    [[nodiscard]] constexpr SolverConfig with_constraint_propagation(const bool v) const { auto c=*this; c.use_constraint_propagation=v; return c; }
    [[nodiscard]] constexpr SolverConfig with_branch_and_bound(const bool v) const       { auto c=*this; c.use_branch_and_bound=v;       return c; }
    [[nodiscard]] constexpr SolverConfig with_cache(const bool v) const                  { auto c=*this; c.use_cache=v;                  return c; }
    [[nodiscard]] constexpr SolverConfig with_precomputed_conflicts(const bool v) const  { auto c=*this; c.use_precomputed_conflicts=v;  return c; }
    [[nodiscard]] constexpr SolverConfig with_dominance_pruning(const bool v) const      { auto c=*this; c.use_dominance_pruning=v;      return c; }
    [[nodiscard]] constexpr SolverConfig with_sequence_squashing(const bool v) const     { auto c=*this; c.use_sequence_squashing=v;     return c; }
    [[nodiscard]] constexpr SolverConfig with_meet_in_the_middle(const bool v) const     { auto c=*this; c.use_meet_in_the_middle=v;     return c; }
    [[nodiscard]] constexpr SolverConfig with_incremental_scoring(const bool v) const    { auto c=*this; c.use_incremental_scoring=v;    return c; }
};

} // namespace detail

// BasicSolverTraits — fluent compile-time builder for solver configuration.
//
// Start from SolverTraits (the default alias) and chain named modifiers:
//
//   SolverTraits
//       ::WithPartialEvaluation<false>
//       ::WithBranchAndBound<true>
//       ::WithPolicies<IntTimePolicy, IntAbsencePolicy>
//
// Order of chaining does not matter.  The resulting type is passed directly
// as the template argument to any solver.

template<detail::SolverConfig Config = {}, policies::Substitutable... Ps>
struct BasicSolverTraits {
    static constexpr detail::SolverConfig config = Config;

    template<bool V> using WithPartialEvaluation     = BasicSolverTraits<Config.with_partial_evaluation(V),     Ps...>;
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

    // Replace the active policy list entirely.
    template<policies::Substitutable... NewPs>
    using WithPolicies = BasicSolverTraits<Config, NewPs...>;

    // Append policies to the existing list.
    template<policies::Substitutable... NewPs>
    using AddPolicies = BasicSolverTraits<Config, Ps..., NewPs...>;
};

// Entry point — begin a chain from here.
using SolverTraits = BasicSolverTraits<>;
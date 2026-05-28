//
// Created by mateu on 15.04.2026.
//

#pragma once

#include <concepts>
#include <ostream>
#include <functional>

class TimeTableState;

namespace solver
{
    struct config
    {
        size_t max_keep_solutions = 10000;
        size_t max_solutions = 10;
        size_t max_runtime = 10000; // time in seconds
        bool verbose = false;
        bool early_stopping = false;
        std::function<void(const TimeTableState& state)> intermediate_solution_callback = [](const TimeTableState& state){};
    };
    inline std::ostream& operator<<(std::ostream& os, const config& c)
    {
        os << "max_keep_solutions: " << c.max_keep_solutions << "\n"
           << "max_solutions:      " << c.max_solutions << "\n"
           << "max_runtime:        " << c.max_runtime << "s\n"
           << "verbose:            " << std::boolalpha << c.verbose << "\n"
           << "early_stopping:     " << c.early_stopping;
        return os;
    }
} // namespace solver

namespace detail {

    // SolverConfig — plain structural aggregate used as a C++20 NTTP.
    //
    // Extracted into its own header so that policies/policies.h can depend on
    // SolverTraitsConcept without pulling in the full traits.h (which would
    // create a circular include: traits.h → policies.h → traits.h).
    struct SolverConfig {
        bool use_partial_evaluation     = false;
        bool use_simplified_evaluation  = true;
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
        bool use_multi_goal_evaluation  = false;

        [[nodiscard]] constexpr SolverConfig with_partial_evaluation(const bool v) const     { auto c=*this; c.use_partial_evaluation=v;     return c; }
        [[nodiscard]] constexpr SolverConfig with_simplified_evaluation(const bool v) const  { auto c=*this; c.use_simplified_evaluation=v;  return c; }
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
        [[nodiscard]] constexpr SolverConfig with_multi_goal_evaluation(const bool v) const    { auto c=*this; c.use_multi_goal_evaluation=v;return c; }
    };

} // namespace detail

// Lightweight concept — only requires a static config member of type SolverConfig.
// Defined here (not in traits.h) so policies.h can use it without a circular include.
template<typename T>
concept SolverTraitsConcept = requires {
    { T::config } -> std::convertible_to<detail::SolverConfig>;
};
//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <constraints.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <solution_set.h>
#include "constraint_evaluator.h"
#include "solver_base.h"
#include "solver_config.h"

#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Compile-time warning helper
//
// Emits a [[deprecated]] warning when BranchAndBoundSolver is instantiated
// with more than 2 OrderSensitive policy types.  The solver still compiles,
// but the runtime will fall back to plain backtracking for any sequence where
// more than 1 OrderSensitive policy is active.
// ---------------------------------------------------------------------------

namespace detail {
    template<bool TooMany>
    struct order_sensitive_policy_count {
        static void check() noexcept {}
    };
    template<>
    struct order_sensitive_policy_count<true> {
        [[deprecated(
            "BranchAndBoundSolver: more than 2 OrderSensitive policy types in Ps... "
            "Only one class order can be active in a sequence; the solver falls back "
            "to plain backtracking whenever more than 1 OrderSensitive policy is present.")]]
        static void check() noexcept {}
    };
} // namespace detail

// ---------------------------------------------------------------------------
// BranchAndBoundSolver<Ps...>
//
// Template B&B solver that accepts any mix of Substitutable policy types.
// It builds a PolicyEvaluator<true, Ps...> internally, so all partial-evaluation
// and bound-estimation infrastructure is available.
//
// Per-sequence behaviour (decided at runtime each sequence):
//
//   n_os == 0   No OrderSensitive policy.  Classes are iterated in default
//               (problem) order.  BoundEstimating policies still provide a
//               lower bound, though its tightness is not guaranteed.
//
//   n_os == 1   Exactly one OrderSensitive policy.  Its class_order() defines
//               the iteration order, ensuring the monotonicity assumption holds
//               for any BoundEstimating policies of the same type.
//
//   n_os >  1   Ambiguous — multiple policies claim different orderings.  Falls
//               back to plain backtracking (no B&B pruning, default order).
//               A compile-time warning is emitted when n_os > 2 at type level.
//
// B&B pruning:
//   At the start of each backtrack node, evaluator_.lower_bound(current) is
//   compared against best_eval (the lowest evaluate() score seen in this
//   sequence).  If lower_bound > best_eval the subtree is pruned — no valid
//   completion can improve on the best solution found so far.
//
//   lower_bound() sums BoundEstimating goals; non-BoundEstimating goals
//   contribute 0 (optimistic assumption).
//
// Weights: soft-goal weights must be non-negative (evaluate = weight * penalty >= 0).
// ---------------------------------------------------------------------------

template<policies::Substitutable... Ps>
class BranchAndBoundSolver : public SolverBase<PolicyEvaluator<true, Ps...>>
{
    using Evaluator = PolicyEvaluator<true, Ps...>;
    using SolverBase<Evaluator>::problem_;
    using SolverBase<Evaluator>::config_;
    using SolverBase<Evaluator>::evaluator_;

    // Number of OrderSensitive types among Ps... (compile-time).
    static constexpr int n_order_sensitive =
        (0 + ... + int(policies::OrderSensitive<Ps>));

public:
    explicit BranchAndBoundSolver(const TimeTableProblem& problem,
                                   const solver::config& config)
        : SolverBase<Evaluator>(problem, config)
    {
        // Compile-time warning when > 2 OrderSensitive types are provided.
        // The [[deprecated]] fires at this call site during template instantiation.
        if constexpr (n_order_sensitive > 2)
            detail::order_sensitive_policy_count<true>::check();
    }

    BoundedSolutionSet<SequenceContext> solve() override;
};

// ---------------------------------------------------------------------------
// solve() — defined inline because BranchAndBoundSolver is header-only.
// ---------------------------------------------------------------------------

template<policies::Substitutable... Ps>
inline BoundedSolutionSet<SequenceContext> BranchAndBoundSolver<Ps...>::solve()
{
    const int    n_classes     = static_cast<int>(problem_.class_size());
    const int    n_seqs        = static_cast<int>(problem_.sequence_size());
    const size_t n_constraints = problem_.get_constraints().size();
    const bool   verbose       = config_.verbose;

    SequenceContext context(n_constraints);
    BoundedSolutionSet<SequenceContext> solutions(config_.max_solutions);

    for (int seq = 0; seq < n_seqs; ++seq)
    {
        solutions.clear();
        evaluator_.set_sequence(seq);

        if (verbose)
            std::cout << "=== Sequence " << seq << " (B&B) ===" << std::endl;

        // ---- Runtime: determine class-iteration order for this sequence ----

        const int n_os = evaluator_.count_order_sensitive();

        if (n_os > 1 && verbose)
            std::cout << "  " << n_os << " OrderSensitive policies active — "
                      << "class order is ambiguous, falling back to plain backtracking.\n";

        const bool use_bnb = (n_os <= 1); // only meaningful B&B when order is unambiguous

        std::vector<int> depth_to_class(n_classes);

        if (use_bnb && n_os == 1)
        {
            const auto order = evaluator_.get_class_order();
            if (static_cast<int>(order.size()) == n_classes)
            {
                for (int d = 0; d < n_classes; ++d)
                    depth_to_class[d] = order[d].first;
            }
            else
            {
                // Malformed order (wrong size) — fall back to default.
                if (verbose)
                    std::cout << "  class_order() returned " << order.size()
                              << " entries (expected " << n_classes
                              << ") — using default order.\n";
                std::iota(depth_to_class.begin(), depth_to_class.end(), 0);
            }
        }
        else
        {
            std::iota(depth_to_class.begin(), depth_to_class.end(), 0);
        }

        // ---- Backtracking search with B&B pruning ----

        int    found_count = 0;
        double best_eval   = std::numeric_limits<double>::infinity();
        TimeTableState current(n_classes);

        std::function<void(int)> backtrack = [&](const int depth)
        {
            // B&B pruning: if the minimum achievable penalty for this branch
            // already strictly exceeds the best known, no completion can improve.
            if (use_bnb)
            {
                const double lb = evaluator_.lower_bound(current);
                if (lb > best_eval) return;
            }

            if (depth == n_classes)
            {
                ++found_count;

                const double eval = evaluator_.evaluate(current);
                if (eval < best_eval) best_eval = eval;

                SequenceContext ctx = evaluator_.score(current);
                evaluator_.update_context(context, current);
                solutions.insert(std::move(ctx), current);

                if (verbose)
                {
                    std::cout << "\r  found: " << found_count
                              << "  best eval: " << best_eval
                              << "    " << std::flush;
                }
                return;
            }

            const int class_id  = depth_to_class[depth];
            const int max_group = problem_.get_max_group(class_id);

            // PolicyEvaluator<true, Ps...> always satisfies PartialConstraintEvaluator.
            auto try_state = [&](const int group)
            {
                if (!evaluator_.partial_are_feasible(current, context, class_id, group))
                { current.unassign(class_id); return; }
                if (!evaluator_.partial_are_satisfied(current, class_id, group))
                { current.unassign(class_id); return; }
                backtrack(depth + 1);
                current.unassign(class_id);
            };

            for (int group = 1; group <= max_group; ++group)
            {
                current.attend(class_id, group);
                try_state(group);

                current.assign(class_id, group);
                try_state(group);
            }
        };

        backtrack(0);

        if (verbose) std::cout << std::endl;

        if (found_count == 0)
        {
            if (verbose)
                std::cout << "  no solutions in sequence " << seq << " — stopping" << std::endl;
            break;
        }
        if (verbose)
            std::cout << "  keeping " << solutions.size() << " solution(s)" << std::endl;
    }

    return solutions;
}

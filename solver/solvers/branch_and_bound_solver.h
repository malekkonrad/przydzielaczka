//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <class_group_range.h>
#include <constraints.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <solution_set.h>
#include "constraint_evaluator.h"
#include "solver_base.h"
#include "../core/include/solver_config.h"
#include "traits.h"

#include <functional>
#include <iostream>
#include <limits>
#include <numeric>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Compile-time helpers
// ---------------------------------------------------------------------------

namespace detail {

    // Compile-time warning when more than 2 OrderSensitive policy types are present.
    template<bool TooMany>
    struct order_sensitive_policy_count {
        static void check() noexcept {}
    };
    template<>
    struct order_sensitive_policy_count<true> {
        [[deprecated(
            "BranchAndBoundSolver: more than 2 OrderSensitive policy types in Traits::WithPolicies<...>. "
            "Only one class order can be active in a sequence; the solver falls back "
            "to plain backtracking whenever more than 1 OrderSensitive policy is present.")]]
        static void check() noexcept {}
    };

    // Count OrderSensitive policies in a BasicSolverTraits specialisation.
    template<SolverTraitsConcept Traits>
    struct order_sensitive_count;

    template<::detail::SolverConfig Config, policies::Substitutable... Ps>
    struct order_sensitive_count<BasicSolverTraits<Config, Ps...>>
        : std::integral_constant<int, (0 + ... + int(policies::OrderSensitive<Ps>))>
    {};

    template<SolverTraitsConcept Traits>
    inline constexpr int order_sensitive_count_v = order_sensitive_count<Traits>::value;

} // namespace detail

// ---------------------------------------------------------------------------
// BranchAndBoundSolver<Traits>
//
// B&B solver that reads all configuration (policies, partial evaluation, …)
// from a SolverTraits type.  Requires Traits::config.use_partial_evaluation
// to be true — partial pruning is fundamental to the B&B algorithm here.
//
// Per-sequence behaviour (decided at runtime each sequence):
//
//   n_os == 0   No OrderSensitive policy.  Classes in default (problem) order.
//               BoundEstimating policies still provide a lower bound.
//
//   n_os == 1   Exactly one OrderSensitive policy.  Its class_order() defines
//               the iteration order, ensuring the monotonicity assumption holds.
//
//   n_os >  1   Ambiguous — multiple policies claim different orderings.  Falls
//               back to plain backtracking (no B&B pruning, default order).
//               A compile-time warning is emitted when n_os > 2.
//
// B&B pruning:
//   At each backtrack node, evaluator_.lower_bound(current) is compared against
//   best_eval.  If lower_bound > best_eval the subtree is pruned.
// ---------------------------------------------------------------------------

template<SolverTraitsConcept Traits = SolverTraits::WithBranchAndBound<true>::WithPartialEvaluation<true>>
class BranchAndBoundSolver : public SolverBase<Traits>
{
    // static_assert(Traits::config.use_partial_evaluation,
    //     "BranchAndBoundSolver requires Traits::config.use_partial_evaluation == true. "
    //     "Use SolverTraits::WithPartialEvaluation<true> in your Traits.");

    using SolverBase<Traits>::problem_;
    using SolverBase<Traits>::config_;
    using SolverBase<Traits>::evaluator_;
    using SolverBase<Traits>::stats_;

    // Number of OrderSensitive policy types (compile-time).
    static constexpr int n_order_sensitive = detail::order_sensitive_count_v<Traits>;

public:
    explicit BranchAndBoundSolver(const TimeTableProblem& problem,
                                   const solver::config& config)
        : SolverBase<Traits>(problem, config)
    {
        // Compile-time warning when > 2 OrderSensitive types are provided.
        if constexpr (n_order_sensitive > 2)
            detail::order_sensitive_policy_count<true>::check();
    }

    BoundedSolutionSet<SequenceContext> solve() override;
};

// ---------------------------------------------------------------------------
// solve() — defined inline because BranchAndBoundSolver is header-only.
// ---------------------------------------------------------------------------

template<SolverTraitsConcept Traits>
BoundedSolutionSet<SequenceContext> BranchAndBoundSolver<Traits>::solve()
{
    const int    n_classes     = static_cast<int>(problem_.class_size());
    const int    n_seqs        = static_cast<int>(problem_.sequence_size());
    const size_t n_constraints = problem_.get_constraints().size();
    const bool   verbose       = config_.verbose;

    SequenceContext context(n_constraints);
    BoundedSolutionSet<SequenceContext> solutions(config_.max_solutions);
    BoundedSolutionSet<double> intermediate_solutions(config_.max_solutions);

    auto class_groups_range = ClassGroupRange(problem_);
    std::vector<std::pair<int,int>> order;

    for (int seq = 0; seq < n_seqs; ++seq)
    {
        bool stop = false;

        solutions.clear();
        evaluator_.set_sequence(seq);

        // ---- Runtime: determine class-iteration order for this sequence ----

        const int n_os = evaluator_.count_order_sensitive();

        if (n_os > 1 && verbose)
            std::cout << "  " << n_os << " OrderSensitive policies active — "
                      << "class order is ambiguous, falling back to plain backtracking.\n";

        bool use_bnb = false;
        if constexpr (Traits::config.use_branch_and_bound)
        {
            use_bnb = (n_os <= 1);
        }

        if (use_bnb && n_os == 1)
        {
            order = evaluator_.get_class_group_order();
            if (order.size() == problem_.size())
            {
                class_groups_range = ClassGroupRange(problem_, order);
            }
            else
            {
                if (verbose)
                {
                    std::cout << "  class_order() returned " << order.size()
                              << " entries (expected " << problem_.size()
                              << ") — using default order." << std::endl;
                }
            }
        }

        this->stats_begin_sequence(seq, this->count_leaves());

        // ---- Backtracking search with B&B pruning ----

        double best_eval = std::numeric_limits<double>::infinity();
        TimeTableState current(n_classes);

        std::function<void(int, int)> backtrack = [&](const int depth, int position)
        {
            if (verbose) this->stats_print_inplace_throttled();

            if (depth == n_classes)
            {
                const double eval = evaluator_.evaluate(current);
                if (eval < best_eval)
                {
                    best_eval = eval;
                }
                this->stats_record_solution(eval);

                SequenceContext ctx = evaluator_.score(current);
                evaluator_.update_context(context, current);
                solutions.insert(std::move(ctx), current);
                intermediate_solutions.insert(eval, current);

                if (config_.early_stopping
                    && intermediate_solutions.worst_score() == best_eval
                    && intermediate_solutions.size() == config_.max_solutions)
                {
                    stop = true;
                }

                if (verbose)
                {
                    this->stats_print_inplace_throttled();
                }
                return;
            }

            if (use_bnb)
            {
                const double lb = evaluator_.lower_bound(current);
                if (lb > best_eval)
                {
                    this->stats_record_pruned(class_groups_range.count_leaves(current, position));
                    return;
                }
            }

            auto try_state = [&](const int class_id, const int group)
            {
                this->stats_record_visited();

                bool are_feasible;
                if constexpr (Traits::config.use_partial_evaluation)
                {
                    are_feasible = evaluator_.partial_are_feasible(current, context, class_id, group);
                }
                else
                {
                    are_feasible = evaluator_.are_feasible(current, context);
                }

                if (!are_feasible)
                {
                    this->stats_record_feasibility_cut(class_groups_range.count_leaves(current, position));
                    current.unassign(class_id);
                    return;
                }

                bool are_satisfied;
                if constexpr (Traits::config.use_partial_evaluation)
                {
                    are_satisfied = evaluator_.partial_are_satisfied(current, class_id, group);
                }
                else
                {
                    are_satisfied = evaluator_.are_satisfied(current);
                }
                if (!are_satisfied)
                {
                    this->stats_record_constraint_cut(class_groups_range.count_leaves(current, position));
                    current.unassign(class_id);
                    return;
                }

                backtrack(depth + 1, position);
            };

            const auto class_groups = class_groups_range.get_class_groups(current, position);

            for (const auto [class_id, group] : class_groups)
            {
                if (stop) return;

                position++;
                current.attend(class_id, group);
                try_state(class_id, group);
                current.unassign(class_id);

                current.assign(class_id, group);
                try_state(class_id, group);
                current.unassign(class_id);
            }

        };

        backtrack(0, 0);

        this->stats_commit_sequence();
        this->stats_set_solutions_kept(static_cast<int>(solutions.size()));

        if (verbose)
        {
            this->current_seq_stats().print_final();
        }

        if (this->current_seq_stats().solutions_found == 0)
        {
            if (verbose)
            {
                std::cout << "  no solutions in sequence " << seq << " — stopping\n";
            }
            break;
        }
        if (verbose)
        {
            std::cout << "  keeping " << solutions.size() << " solution(s)\n";
        }
    }

    if (verbose)
    {
        stats_.print();
    }

    return solutions;
}

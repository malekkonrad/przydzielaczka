//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <constraints.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include "constraint_evaluator.h"
#include "solver_base.h"

#include <functional>
#include <iostream>
#include <utility>
#include <vector>


// BranchAndBoundSolver — a concrete, non-template backtracking solver.
//
// Algorithm (one pass per sequence):
//   1. Backtrack over all classes, pruning on time overlap.
//   2. At the leaf, reject states that violate any hard constraint for this
//      sequence or fail the slack-feasibility check from the previous sequence.
//   3. Score accepted states by the soft goals of the current sequence.
//   4. Build a SequenceContext from the minimum per-constraint penalty across
//      all optimal-score states (most permissive bound for the next sequence).
//   5. Repeat for the next sequence.
//
// This is the baseline: correctness over performance.
// Constraint-level pruning (early backtracking) is left for derived solvers.
class BranchAndBoundSolver : public SolverBase<BaseEvaluator>
{
public:
    explicit BranchAndBoundSolver(const TimeTableProblem& problem, const solver::config& config)
        : SolverBase<BaseEvaluator>(problem, config) {}

    BoundedSolutionSet<SequenceContext> solve() override;
};

// TODO for branch and bound the weights should always be positive and that creates a problem with deselecting a lecturer.

// ---------------------------------------------------------------------------
// solve() — defined inline here because BranchAndBoundSolver is header-only.
// ---------------------------------------------------------------------------

inline BoundedSolutionSet<SequenceContext> BranchAndBoundSolver::solve()
{
    const int n_classes = static_cast<int>(problem_.class_size());
    const int n_seqs    = static_cast<int>(problem_.sequence_size());
    const size_t n_constraints = problem_.get_constraints().size();
    const bool verbose  = config_.verbose;

    SequenceContext context(n_constraints); // empty — no prior sequence
    BoundedSolutionSet<SequenceContext> solutions(config_.max_solutions);

    for (int seq = 0; seq < n_seqs; ++seq)
    {
        bool stop = false;

        solutions.clear();
        evaluator_.set_sequence(seq);
        if (verbose)
        {
            std::cout << "=== Sequence " << seq << " ===" << std::endl;
        }

        int found_count = 0;
        TimeTableState current(n_classes);

        std::function<void(int)> backtrack = [&](const int depth)
        {
            if (depth == n_classes)
            {
                ++found_count;
                SequenceContext ctx = evaluator_.score(current);
                evaluator_.update_context(context, current);
                solutions.insert(std::move(ctx), current);
                if (verbose)
                {
                    const double best = solutions.empty() ? context.sum() : solutions.best_score().sum();
                    std::cout << "\r  found: " << found_count
                              << "  best score: " << best
                              << "    " << std::flush;
                }
                // // early stopping when solutions are full
                // if (solutions.worst_score().sum() == 0.0) stop = true;

                return;
            }

            const int class_id = depth;
            const int max_group = problem_.get_max_group(class_id);

            auto try_state = [&]()
            {
                if (!evaluator_.are_feasible(current, context))
                {
                    current.unassign(class_id);
                    return;
                }
                if (!evaluator_.are_satisfied(current))
                {
                    current.unassign(class_id);
                    return;
                }
                backtrack(depth + 1);
                current.unassign(class_id);
            };

            for (int group = 1; group <= max_group && !stop; ++group)
            {
                current.attend(class_id, group);
                try_state();

                current.assign(class_id, group);
                try_state();
            }
        };

        backtrack(0);

        if (verbose)
        {
            std::cout << std::endl; // close the \r line
        }

        if (found_count == 0)
        {
            if (verbose)
            {
                std::cout << "  no solutions in sequence " << seq << " — stopping" << std::endl;
            }
            break;
        }

        if (verbose)
        {
            std::cout << "  keeping " << solutions.size() << " solution(s)" << std::endl;
        }
    }

    return solutions;
}

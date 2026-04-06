//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <constraints.h>
#include <solver_base.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <constraint_evaluator.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>
#include <utility>
#include <ranges>
#include <vector>

#include "constraint_evaluator.h"
#include "solver_base.h"

// OptimizedFullSolver — a template backtracking solver that accepts any Evaluatable.
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
// Extra constructor arguments are forwarded to the Evaluator, so policies can
// be injected: OptimizedFullSolver<PolicyConstraintEvaluator<IntTimePolicy>>(problem, cfg, policies)
template<typename Evaluator>
    requires ConstraintEvaluator<Evaluator>
class OptimizedFullSolver : public SolverBase<Evaluator>
{
    using SolverBase<Evaluator>::problem_;
    using SolverBase<Evaluator>::config_;
    using SolverBase<Evaluator>::evaluator_;

public:
    template<typename... Args>
    explicit OptimizedFullSolver(const TimeTableProblem& problem, const solver::config& config,
                                 Args&&... args)
        : SolverBase<Evaluator>(problem, config, std::forward<Args>(args)...) {}

    std::vector<TimeTableState> solve() override;

    struct ScoreOnly {
        bool operator()(const std::pair<double, TimeTableState>& a,
                        const std::pair<double, TimeTableState>& b) const
        {
            return a.first < b.first;
        }
    };

    // Sorted set: (score, state), ascending by score. Worst = last element.
    using SolutionSet = std::multiset<std::pair<double, TimeTableState>, ScoreOnly>;

private:
    void add_solution(SolutionSet& solutions, const TimeTableState& state, const double score)
    {
        if (solutions.size() < config_.max_solutions)
        {
            solutions.emplace(score, state);
        }
        else if (score < solutions.rbegin()->first)
        {
            solutions.erase(std::prev(solutions.end()));
            solutions.emplace(score, state);
        }
    }
};

// ---------------------------------------------------------------------------
// solve() — defined inline here because OptimizedFullSolver is header-only.
// ---------------------------------------------------------------------------

template<typename Evaluator>
    requires ConstraintEvaluator<Evaluator>
inline std::vector<TimeTableState> OptimizedFullSolver<Evaluator>::solve()
{
    const int n_classes = static_cast<int>(problem_.class_size());
    const int n_seqs    = static_cast<int>(problem_.sequence_size());
    const size_t n_constraints = problem_.get_constraints().size();
    const bool verbose  = config_.verbose;

    SequenceContext context(n_constraints); // empty — no prior sequence
    SolutionSet solutions;

    for (int seq = 0; seq < n_seqs; ++seq)
    {
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
                evaluator_.update_context(context, current);
                const double score = evaluator_.evaluate(current);
                add_solution(solutions, current, score);
                if (verbose)
                {
                    const double best = solutions.empty() ? score : solutions.begin()->first;
                    std::cout << "\r  found: " << found_count
                              << "  best score: " << best
                              << "    " << std::flush;
                }
                return;
            }

            const int class_id = depth;
            const int max_group = problem_.get_max_group(class_id);

            auto try_state = [&](const int group)
            {
                if constexpr (PartialConsraintEvaluator<Evaluator>)
                {
                    if (!evaluator_.partial_are_feasible(current, context, class_id, group))
                    {
                        current.unassign(class_id);
                        return;
                    }
                    if (!evaluator_.partial_are_satisfied(current, class_id, group))
                    {
                        current.unassign(class_id);
                        return;
                    }
                }
                else
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
                }
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

    std::vector<TimeTableState> result;
    result.reserve(solutions.size());
    for (const auto& state : solutions | std::views::values)
        result.push_back(state);
    return result;
}
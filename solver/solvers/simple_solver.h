//
// Created by mateu on 01.04.2026.
//

#pragma once

#include "solver_base.h"

template<typename Evaluator>
class SimpleSolver : public SolverBase<Evaluator>
{
public:
    explicit SimpleSolver(const TimeTableProblem& problem)
        : SolverBase<Evaluator>(problem) {}

    std::vector<TimeTableState> solve(int max_solutions = 10) override;
};

// Implementation lives here because SimpleSolver is a template.
// Explicit instantiations for known evaluators are in simple_solver.cpp.
template<typename Evaluator>
std::vector<TimeTableState> SimpleSolver<Evaluator>::solve(int /*max_solutions*/)
{
    // Use this->evaluator_.has_conflict(candidate_id, state) to prune.
    // Use this->evaluator_.score(state) to rank complete solutions.
    return {};
}

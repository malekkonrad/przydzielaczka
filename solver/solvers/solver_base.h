//
// Created by mateu on 01.04.2026.
//

#pragma once

#include "i_solver.h"

#include <time_table_problem.h>
#include <time_table_state.h>

// SolverBase<Evaluator> sits between ISolver and concrete solvers.
// It constructs the evaluator and exposes it to subclasses.
// All conflict checking and scoring goes through the evaluator.
//
// Evaluator must provide:
//   explicit Evaluator(const TimeTableProblem&);
//   bool   has_conflict(int candidate_id, const TimeTableState&) const;
//   double score(const TimeTableState&) const;
template<typename Evaluator>
class SolverBase : public ISolver
{
public:
    explicit SolverBase(const TimeTableProblem& problem)
        : ISolver(problem), evaluator_(problem) {}

protected:
    Evaluator evaluator_;
};

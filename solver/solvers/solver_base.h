//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <time_table_problem.h>
#include <time_table_state.h>
#include "solver_config.h"

#include <vector>


// SolverBase<Evaluator> is the base for concrete solvers.
// It owns the problem reference, constructs the evaluator, and exposes both
// to subclasses. All conflict checking and scoring goes through the evaluator.
//
// Evaluator must provide:
//   explicit Evaluator(const TimeTableProblem&);
//   bool   has_conflict(int candidate_id, const TimeTableState&) const;
//   double score(const TimeTableState&) const;
template<typename Evaluator>
class SolverBase
{
public:
    explicit SolverBase(const TimeTableProblem& problem, const solver::config& config)
        : problem_(problem), config_(config), evaluator_(problem) {}

    virtual ~SolverBase() = default;

    SolverBase(const SolverBase&)            = delete;
    SolverBase& operator=(const SolverBase&) = delete;

    virtual std::vector<TimeTableState> solve() = 0;

protected:
    const TimeTableProblem& problem_;
    const solver::config& config_;
    Evaluator evaluator_;
};

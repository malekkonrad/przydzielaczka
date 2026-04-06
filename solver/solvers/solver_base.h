//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <time_table_problem.h>
#include <time_table_state.h>
#include <constraint_evaluator.h>
#include "solver_config.h"

#include <utility>
#include <vector>


// SolverBase<Evaluator> is the base for concrete solvers.
// It owns the problem reference, constructs the evaluator, and exposes both
// to subclasses. All conflict checking and scoring goes through the evaluator.
//
// Evaluator must satisfy the Evaluatable concept:
//   score / update_context / evaluate / are_satisfied / are_feasible
//
// The variadic constructor forwards extra arguments to the Evaluator constructor,
// allowing policies and other configuration to be passed through.
template<typename Evaluator = BaseEvaluator>
    requires concepts::ConstraintEvaluator<Evaluator>
class SolverBase
{
public:
    template<typename... Args>
    explicit SolverBase(const TimeTableProblem& problem, const solver::config& config,
                        Args&&... args)
        : problem_(problem), config_(config),
          evaluator_(problem, std::forward<Args>(args)...) {}

    virtual ~SolverBase() = default;

    SolverBase(const SolverBase&)            = delete;
    SolverBase& operator=(const SolverBase&) = delete;

    virtual std::vector<TimeTableState> solve() = 0;

protected:
    const TimeTableProblem& problem_;
    const solver::config& config_;
    Evaluator evaluator_;
};

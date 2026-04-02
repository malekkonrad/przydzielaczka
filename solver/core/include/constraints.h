//
// Created by mateu on 30.03.2026.
//

#pragma once

#include "data_models.h"
#include "time_table_state.h"

class TimeTableProblem;

namespace constraints {

    // Returns a penalty for a single constraint (0 = fully satisfied, positive = violated).
    double evaluate(
        const solver_models::ConstraintVariant& constraint,
        const TimeTableState& state,
        const TimeTableProblem& problem
    );

    // Returns the total weighted penalty for the entire state.
    // Hard constraints that are violated contribute 1e9 each.
    // Lower score is better.
    double evaluate_all(
        const TimeTableProblem& problem,
        const TimeTableState& state
    );

} // namespace constraints

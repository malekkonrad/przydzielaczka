//
// Shared test helpers: factory functions for building solver data models.
//
// Keep these functions thin wrappers — they only set the fields the test cares
// about and use sensible defaults for everything else.  If the data model
// changes, update this file in one place rather than every test.
//

#pragma once

#include "solver_data_models.h"
#include "time_table_problem.h"
#include "time_table_state.h"

#include <vector>

namespace test {

// ---- Week helpers ----

/// Both weeks A and B active (bits 0 and 1 set).
inline solver_models::TimeTableWeek both_weeks()
{
    solver_models::TimeTableWeek w;
    w.set(0);
    w.set(1);
    return w;
}

/// Only week A active (bit 0 set).
inline solver_models::TimeTableWeek week_a()
{
    solver_models::TimeTableWeek w;
    w.set(0);
    return w;
}

/// Only week B active (bit 1 set).
inline solver_models::TimeTableWeek week_b()
{
    solver_models::TimeTableWeek w;
    w.set(1);
    return w;
}

// ---- Class factory ----

/// Creates a Class with the given id and time window.
/// All other fields default to 0 / both_weeks().
inline solver_models::Class make_class(
    int id,
    int day              = 0,
    solver_models::TimeTableWeek week = {},   // set in body
    int start_time       = 800,
    int end_time         = 1000,
    int group            = 0,
    int lecturer         = 0,
    int location         = 0)
{
    if (week.none()) week = both_weeks();
    return solver_models::Class{
        .id         = id,
        .lecturer   = lecturer,
        .day        = day,
        .week       = week,
        .location   = location,
        .group      = group,
        .start_time = start_time,
        .end_time   = end_time,
        .sessions   = {}
    };
}

// ---- Problem and state factories ----

inline TimeTableProblem make_problem(
    std::vector<solver_models::Class> classes = {},
    std::vector<solver_models::ConstraintVariant> constraints = {})
{
    return TimeTableProblem(std::move(classes), std::move(constraints));
}

inline TimeTableState make_state(std::vector<int> ids = {})
{
    return TimeTableState(std::move(ids));
}

} // namespace test

//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <time_table_problem.h>
#include <time_table_state.h>

#include <vector>

class ISolver
{
public:
    explicit ISolver(const TimeTableProblem& problem) : problem_(problem) {}
    virtual ~ISolver() = default;

    ISolver(const ISolver&)            = delete;
    ISolver& operator=(const ISolver&) = delete;

    virtual std::vector<TimeTableState> solve(int max_solutions = 10) = 0;

protected:
    const TimeTableProblem& problem_;
};

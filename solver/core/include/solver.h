//
// Created by mateu on 25.03.2026.
//

#pragma once

#include <iosfwd>
#include <vector>

#include "time_table_problem.h"
#include "time_table_state.h"

class Solver
{
public:
    explicit Solver(const TimeTableProblem& problem);

    // Runs backtracking and returns up to max_solutions best solutions sorted by
    // score (lowest weighted penalty first). Stops collecting after
    // MAX_COLLECTED_SOLUTIONS to avoid exponential blowup on large inputs.
    std::vector<TimeTableState> solve(int max_solutions = 10);

private:
    const TimeTableProblem& problem_;

    static constexpr int MAX_COLLECTED_SOLUTIONS = 10'000;

    void backtrack(
        int class_index,
        TimeTableState& current,
        std::vector<TimeTableState>& solutions
    ) const;

    // Returns true if cls overlaps in time with any already-chosen class.
    bool has_time_conflict(const solver_models::Class& cls, const TimeTableState& state) const;

    friend std::ostream& operator<<(std::ostream& out, const Solver& s);
};

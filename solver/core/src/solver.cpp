//
// Created by mateu on 25.03.2026.
//

#include "solver.h"
#include "constraints.h"

#include <algorithm>
#include <ostream>

Solver::Solver(const TimeTableProblem& problem)
    : problem_(problem) {}

// -------------------- PUBLIC --------------------

std::vector<TimeTableState> Solver::solve(int max_solutions)
{
    std::vector<TimeTableState> solutions;
    TimeTableState current;
    backtrack(0, current, solutions);

    // Sort by score ascending (lower penalty = better).
    std::sort(solutions.begin(), solutions.end(),
        [this](const TimeTableState& a, const TimeTableState& b) {
            return constraints::evaluate_all(problem_, a) < constraints::evaluate_all(problem_, b);
        });

    if (static_cast<int>(solutions.size()) > max_solutions)
        solutions.resize(max_solutions);

    return solutions;
}

// -------------------- PRIVATE --------------------

void Solver::backtrack(int index, TimeTableState& current,
                       std::vector<TimeTableState>& solutions) const
{
    // Hard limit to prevent memory/time blowup on large inputs.
    if (static_cast<int>(solutions.size()) >= MAX_COLLECTED_SOLUTIONS)
        return;

    if (index == static_cast<int>(problem_.get_classes().size()))
    {
        // Leaf node: accept solution only if no hard constraint is violated.
        if (constraints::evaluate_all(problem_, current) < 1e9)
            solutions.push_back(current);
        return;
    }

    const solver_models::Class& cls = problem_.get_classes()[index];

    // Branch 1: skip this class.
    backtrack(index + 1, current, solutions);

    // Branch 2: include this class only when there is no time conflict.
    if (!has_time_conflict(cls, current))
    {
        current.add(cls.id);
        backtrack(index + 1, current, solutions);
        current.remove(cls.id);
    }
}

bool Solver::has_time_conflict(const solver_models::Class& cls,
                               const TimeTableState& state) const
{
    for (int id : state.get_chosen_ids())
    {
        const solver_models::Class* other = problem_.find_class(id);
        if (!other) continue;
        if (cls.day != other->day) continue;
        // Weeks must overlap (share at least one week bit).
        if ((cls.week & other->week).none()) continue;
        // Time intervals must overlap.
        if (cls.start_time < other->end_time && other->start_time < cls.end_time)
            return true;
    }
    return false;
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const Solver& s)
{
    out << "Solver{ algorithm=backtracking"
        << ", classes=" << s.problem_.get_classes().size()
        << ", constraints=" << s.problem_.get_constraints().size()
        << ", max_collected=" << Solver::MAX_COLLECTED_SOLUTIONS
        << " }";
    return out;
}

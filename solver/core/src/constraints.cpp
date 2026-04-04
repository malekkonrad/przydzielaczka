//
// Created by mateu on 30.03.2026.
//

#include "constraints.h"
#include "time_table_problem.h"

#include <algorithm>
#include <map>
#include <variant>
#include <vector>

using namespace solver_models;

namespace constraints {

// -------------------- INDIVIDUAL EVALUATORS --------------------

static double eval(const MinimizeGapsConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // Group chosen classes by day, then penalize gaps larger than min_break.
    std::map<int, std::vector<const Class*>> by_day;
    // for (int id : state.get_chosen_ids())
    // {
    //     const Class* cls = problem.find_class(id);
    //     if (cls) by_day[cls->day].push_back(cls);
    // }

    double penalty = 0.0;
    // for (auto& [day, classes] : by_day)
    // {
    //     std::sort(classes.begin(), classes.end(),
    //         [](const Class* a, const Class* b) { return a->start_time < b->start_time; });
    //
    //     for (std::size_t i = 1; i < classes.size(); ++i)
    //     {
    //         int gap = classes[i]->start_time - classes[i - 1]->end_time;
    //         if (gap > c.min_break)
    //             penalty += static_cast<double>(gap - c.min_break);
    //     }
    // }
    return penalty;
}

static double eval(const GroupPreferenceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // if (!state.contains(c.class_id)) return 0.0;
    // const Class* cls = problem.find_class(c.class_id);
    // if (!cls) return 0.0;
    // return cls->group == c.preferred_group ? 0.0 : 1.0;
    return 0.0;
}

static double eval(const LecturerPreferenceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // if (!state.contains(c.class_id)) return 0.0;
    // const Class* cls = problem.find_class(c.class_id);
    // if (!cls) return 0.0;
    // return cls->lecturer == c.lecturer ? 0.0 : 1.0;
    return 0.0;
}

static double eval(const MaximizeClassAttendanceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& /*problem*/)
{
    return state.is_assigned(c.class_id) ? 0.0 : 1.0;
}

static double eval(const MaximizeGroupAttendanceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& /*problem*/)
{
    return state.is_assigned(c.class_id) ? 0.0 : 1.0;
}

static double eval(const MaximizeTotalAttendanceConstraint& /*c*/,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    int total = static_cast<int>(problem.get_classes().size());
    if (total == 0) return 0.0;
    // Penalty = fraction of classes not attended (in [0, 1]).
    return static_cast<double>(total - state.size()) / total;
}

static double eval(const TimeBlockDayConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    double penalty = 0.0;
    // for (int id : state.get_chosen_ids())
    // {
    //     const Class* cls = problem.find_class(id);
    //     if (!cls || cls->day != c.day) continue;
    //     // Overlapping time intervals: [cls->start, cls->end) ∩ [c.start, c.end)
    //     if (cls->start_time < c.end_time && c.start_time < cls->end_time)
    //         penalty += 1.0;
    // }
    return penalty;
}

static double eval(const TimeBlockDateConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    double penalty = 0.0;
    // for (int id : state.get_chosen_ids())
    // {
    //     const Class* cls = problem.find_class(id);
    //     if (!cls) continue;
    //     for (const auto& session : cls->sessions)
    //     {
    //         if (session.date != c.date) continue;
    //         if (session.start_time < c.end_time && c.start_time < session.end_time)
    //         {
    //             penalty += 1.0;
    //             break; // count the class once, not once per session
    //         }
    //     }
    // }
    return penalty;
}

static double eval(const PreferEdgeClassConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // if (!state.contains(c.class_id)) return 0.0;
    // const Class* cls = problem.find_class(c.class_id);
    // if (!cls) return 0.0;
    //
    // // Find the earliest start and latest end among chosen classes on the same day.
    // int earliest = cls->start_time;
    // int latest   = cls->end_time;
    // // for (int id : state.get_chosen_ids())
    // // {
    // //     const Class* other = problem.find_class(id);
    // //     if (!other || other->day != cls->day) continue;
    // //     earliest = std::min(earliest, other->start_time);
    // //     latest   = std::max(latest,   other->end_time);
    // // }
    //
    // if (c.position == EdgePosition::Start && cls->start_time == earliest) return 0.0;
    // if (c.position == EdgePosition::End   && cls->end_time   == latest)   return 0.0;
    return 1.0;
}

static double eval(const PreferEdgeGroupConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // if (!state.contains(c.class_id)) return 0.0;
    // const Class* cls = problem.find_class(c.class_id);
    // if (!cls) return 0.0;
    //
    // // Find the earliest start and latest end among chosen classes on the same day.
    // int earliest = cls->start_time;
    // int latest   = cls->end_time;
    // // for (int id : state.get_chosen_ids())
    // // {
    // //     const Class* other = problem.find_class(id);
    // //     if (!other || other->day != cls->day) continue;
    // //     earliest = std::min(earliest, other->start_time);
    // //     latest   = std::max(latest,   other->end_time);
    // // }
    //
    // if (c.position == EdgePosition::Start && cls->start_time == earliest) return 0.0;
    // if (c.position == EdgePosition::End   && cls->end_time   == latest)   return 0.0;
    return 1.0;
}

// -------------------- PUBLIC API --------------------

double evaluate(const ConstraintVariant& constraint,
                const TimeTableState& state,
                const TimeTableProblem& problem)
{
    return std::visit([&](const auto& c) {
        return eval(c, state, problem);
    }, constraint);
}

double evaluate_all(const TimeTableProblem& problem, const TimeTableState& state)
{
    double total = 0.0;
    for (const auto& constraint : problem.get_constraints())
    {
        double penalty = evaluate(constraint, state, problem);
        std::visit([&](const auto& c) {
            if (c.hard && penalty > 0.0)
                total += 1e9;
            else
                total += c.weight * penalty;
        }, constraint);
    }
    return total;
}

} // namespace constraints

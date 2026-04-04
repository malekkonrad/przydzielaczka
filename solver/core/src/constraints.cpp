//
// Created by mateu on 30.03.2026.
//

#include "constraints.h"
#include "time_table_problem.h"

#include <algorithm>
#include <map>
#include <variant>
#include <vector>

using namespace solver_models;  // TODO remove

namespace constraints {

// -------------------- INDIVIDUAL EVALUATORS --------------------

static double eval(const MinimizeGapsConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    // Group chosen classes by (day, week_bit), then penalize gaps > min_break.
    // week_bit: 0 = week A, 1 = week B.
    std::map<std::pair<int,int>, std::vector<const Class*>> by_day_week;
    const auto& groups = state.get_groups();
    for (int class_id = 0; class_id < static_cast<int>(groups.size()); ++class_id)
    {
        if (groups[class_id] < 0) continue;
        const auto& cls = problem.get_class(class_id, groups[class_id]);
        for (int wi = 0; wi < 2; ++wi)
        {
            if (cls.week.test(wi))
                by_day_week[{cls.day, wi}].push_back(&cls);
        }
    }

    double penalty = 0.0;
    for (auto& [key, day_classes] : by_day_week)
    {
        std::sort(day_classes.begin(), day_classes.end(),
            [](const Class* a, const Class* b) { return a->start_time < b->start_time; });

        for (std::size_t i = 1; i < day_classes.size(); ++i)
        {
            const int gap = day_classes[i]->start_time - day_classes[i - 1]->end_time;
            if (gap > c.min_break)
                penalty += static_cast<double>(gap - c.min_break);
        }
    }
    return penalty;
}

static double eval(const GroupPreferenceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& /*problem*/)
{
    if (!state.is_assigned(c.class_id)) return 0.0;
    return state.is_assigned(c.class_id, c.group) ? 0.0 : 1.0;
}

static double eval(const LecturerPreferenceConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    if (!state.is_assigned(c.class_id)) return 0.0;
    const int group = state.get_groups()[c.class_id];
    return problem.get_class(c.class_id, group).lecturer == c.lecturer ? 0.0 : 1.0;
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
    return state.is_assigned(c.class_id, c.group) ? 0.0 : 1.0;
}

static double eval(const MaximizeTotalAttendanceConstraint& /*c*/,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    const int total = static_cast<int>(problem.class_size());
    if (total == 0) return 0.0;
    return static_cast<double>(total - static_cast<int>(state.filled())) / total;
}

static double eval(const TimeBlockDayConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    double penalty = 0.0;
    const auto& groups = state.get_groups();
    for (int class_id = 0; class_id < static_cast<int>(groups.size()); ++class_id)
    {
        if (groups[class_id] < 0) continue;
        const auto& cls = problem.get_class(class_id, groups[class_id]);
        if (cls.day != c.day) continue;
        if (cls.start_time < c.end_time && c.start_time < cls.end_time)
            penalty += 1.0;
    }
    return penalty;
}

static double eval(const TimeBlockDateConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    double penalty = 0.0;
    const auto& groups = state.get_groups();
    for (int class_id = 0; class_id < static_cast<int>(groups.size()); ++class_id)
    {
        if (groups[class_id] < 0) continue;
        const auto& cls = problem.get_class(class_id, groups[class_id]);
        for (const auto& session : cls.sessions)
        {
            if (session.date != c.date) continue;
            if (session.start_time < c.end_time && c.start_time < session.end_time)
            {
                penalty += 1.0;
                break; // count the class once, not once per session
            }
        }
    }
    return penalty;
}

static double eval(const PreferEdgeClassConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    if (!state.is_assigned(c.class_id)) return 0.0;
    const int group = state.get_groups()[c.class_id];
    const auto& cls = problem.get_class(c.class_id, group);

    int earliest = cls.start_time;
    int latest   = cls.end_time;
    const auto& groups = state.get_groups();
    for (int other_id = 0; other_id < static_cast<int>(groups.size()); ++other_id)
    {
        if (groups[other_id] < 0) continue;
        const auto& other = problem.get_class(other_id, groups[other_id]);
        if (other.day != cls.day) continue;
        earliest = std::min(earliest, other.start_time);
        latest   = std::max(latest,   other.end_time);
    }

    if (c.position == EdgePosition::Start && cls.start_time == earliest) return 0.0;
    if (c.position == EdgePosition::End   && cls.end_time   == latest)   return 0.0;
    return 1.0;
}

static double eval(const PreferEdgeGroupConstraint& c,
                   const TimeTableState& state,
                   const TimeTableProblem& problem)
{
    if (!state.is_assigned(c.class_id, c.group)) return 0.0;
    const auto& cls = problem.get_class(c.class_id, c.group);

    int earliest = cls.start_time;
    int latest   = cls.end_time;
    const auto& groups = state.get_groups();
    for (int other_id = 0; other_id < static_cast<int>(groups.size()); ++other_id)
    {
        if (groups[other_id] < 0) continue;
        const auto& other = problem.get_class(other_id, groups[other_id]);
        if (other.day != cls.day) continue;
        earliest = std::min(earliest, other.start_time);
        latest   = std::max(latest,   other.end_time);
    }

    if (c.position == EdgePosition::Start && cls.start_time == earliest) return 0.0;
    if (c.position == EdgePosition::End   && cls.end_time   == latest)   return 0.0;
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
        const double penalty = evaluate(constraint, state, problem);
        std::visit([&](const auto& c){
            if (c.hard && penalty > 0.0)
                total += 1e9;
            else
                total += c.weight * penalty;
        }, constraint);
    }
    return total;
}

} // namespace constraints
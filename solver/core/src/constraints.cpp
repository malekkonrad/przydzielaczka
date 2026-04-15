//
// Created by mateu on 30.03.2026.
//

#include "constraints.h"
#include "data_models.h"
#include "time_table_problem.h"

#include <algorithm>
#include <map>
#include <sequence_context.h>
#include <variant>
#include <vector>
#include <iostream>

using namespace solver_models;

// TODO right now only single date is taken into account, implement full later

// Verify at compile time that every variant alternative satisfies Evaluatable.
// This fires with a clear error if a new constraint type is added without
// implementing penalty(), evaluate(), or is_satisfied().
static_assert(
    constraints::all_evaluatable_v<solver_models::ConstraintVariant>,
    "Every type in ConstraintVariant must implement: penalty(), evaluate(), is_satisfied(), and is_feasible()\n"
    "As well as public atributes: id, hard, sequence, type"
);

// -------------------- MinimizeGapsConstraint --------------------

double MinimizeGapsConstraint::penalty(const TimeTableState& state,
                                       const TimeTableProblem& problem) const
{
    struct TimeEntry
    {
        int start_time;
        int end_time;
    };

    std::vector<std::vector<TimeEntry>> by_date(problem.get_max_date() + 1, std::vector<TimeEntry>());

    const std::function<bool(const TimeEntry&, const TimeEntry&)> class_time_cmp = [](const TimeEntry& a, const TimeEntry& b)
    {
        if (a.start_time == b.start_time)
        {
            return a.end_time < b.end_time;
        }
        return a.start_time < b.start_time;
    };

    for (int class_id = 0; class_id < state.size(); ++class_id)
    {
        if (!state.is_attended(class_id))
        {
            continue;
        }

        const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
        for (const auto& s : cls.sessions)
        {
            auto& day_classes = by_date[s.date];
            day_classes.emplace_back(s.start_time, s.end_time);
        }
    }

    double breaks = 0.0;
    for (auto& day_classes : by_date)
    {
        if (day_classes.size() < 2)
        {
            continue;
        }

        std::ranges::sort(day_classes, class_time_cmp);

        for (size_t i = 1; i < day_classes.size(); ++i)
        {
            const int gap = day_classes[i].start_time - day_classes[i - 1].end_time;
            if (gap > min_break)
            {
                breaks += static_cast<double>(gap - min_break);
            }
        }
    }
    breaks /= static_cast<double>(by_date.size());
    breaks *= problem.get_max_day() + 1;
    return breaks;

    // ====== OLD PER WEEK GAPS =========
    // std::map<std::pair<int,int>, std::vector<const Class*>> by_day_week;
    // for (int class_id = 0; class_id < state.size(); ++class_id)
    // {
    //     if (!state.is_attended(class_id))
    //     {
    //         continue;
    //     }
    //     const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
    //     if (cls.week.test(0))
    //     {
    //         by_day_week[{cls.day, 0}].push_back(&cls);
    //     }
    //     if (cls.week.test(1))
    //     {
    //         by_day_week[{cls.day, 1}].push_back(&cls);
    //     }
    // }
    //
    // double penalty = 0.0;
    // for (auto& day_classes : by_day_week | std::views::values)
    // {
    //     std::sort(day_classes.begin(), day_classes.end(),
    //         [](const Class* a, const Class* b)
    //         {
    //             if (a->start_time == b->start_time)
    //             {
    //                 return a->end_time < b->end_time;
    //             }
    //             return a->start_time < b->start_time;
    //         });
    //
    //     for (size_t i = 1; i < day_classes.size(); ++i)
    //     {
    //         const int gap = day_classes[i]->start_time - day_classes[i - 1]->end_time;
    //         if (gap > min_break)
    //         {
    //             penalty += static_cast<double>(gap - min_break);
    //         }
    //     }
    // }
    // return penalty;
}

double MinimizeGapsConstraint::evaluate(const TimeTableState& state,
                                        const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool MinimizeGapsConstraint::is_satisfied(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool MinimizeGapsConstraint::is_feasible(const TimeTableState& state,
                                         const TimeTableProblem& problem,
                                         const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- GroupPreferenceConstraint --------------------

double GroupPreferenceConstraint::penalty(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id, group) ? 0.0 : 1.0;
}

double GroupPreferenceConstraint::evaluate(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool GroupPreferenceConstraint::is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool GroupPreferenceConstraint::is_feasible(const TimeTableState& state,
                                            const TimeTableProblem& problem,
                                            const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- LecturerPreferenceConstraint --------------------

double LecturerPreferenceConstraint::penalty(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    if (!state.is_attended(class_id)) return 1.0;
    const int group = state.get_raw_group(class_id);
    return problem.get_group(class_id, group).lecturer == lecturer ? 0.0 : 1.0;
}

double LecturerPreferenceConstraint::evaluate(const TimeTableState& state,
                                              const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool LecturerPreferenceConstraint::is_satisfied(const TimeTableState& state,
                                                const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool LecturerPreferenceConstraint::is_feasible(const TimeTableState& state,
                                               const TimeTableProblem& problem,
                                               const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeClassAbsenceConstraint --------------------

double MinimizeClassAbsenceConstraint::penalty(const TimeTableState& state,
                                                  const TimeTableProblem& /*problem*/) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id) ? 0.0 : 1.0;
}

double MinimizeClassAbsenceConstraint::evaluate(const TimeTableState& state,
                                                   const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool MinimizeClassAbsenceConstraint::is_satisfied(const TimeTableState& state,
                                                     const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool MinimizeClassAbsenceConstraint::is_feasible(const TimeTableState& state,
                                                    const TimeTableProblem& problem,
                                                    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeGroupAbsenceConstraint --------------------

double MinimizeGroupAbsenceConstraint::penalty(const TimeTableState& state,
                                                  const TimeTableProblem& /*problem*/) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id, group) ? 0.0 : 1.0;
}

double MinimizeGroupAbsenceConstraint::evaluate(const TimeTableState& state,
                                                   const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool MinimizeGroupAbsenceConstraint::is_satisfied(const TimeTableState& state,
                                                     const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool MinimizeGroupAbsenceConstraint::is_feasible(const TimeTableState& state,
                                                    const TimeTableProblem& problem,
                                                    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeTotalAbsenceConstraint --------------------

double MinimizeTotalAbsenceConstraint::penalty(const TimeTableState& state,
                                               const TimeTableProblem& problem) const
{
    struct TimeEntry
    {
        int class_id;
        int start_time;
        int end_time;
    };

    std::vector<std::vector<TimeEntry>> by_date(problem.get_max_date() + 1, std::vector<TimeEntry>());

    for (int class_id = 0; class_id < state.size(); ++class_id)
    {
        if (!state.is_attended(class_id))
        {
            continue;
        }

        const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
        for (const auto& s : cls.sessions)
        {
            auto& day_classes = by_date[s.date];
            day_classes.emplace_back(class_id, s.start_time, s.end_time);
        }
    }

    std::vector<double> attendance(state.size(), 0.0);
    for (auto& day_classes : by_date)
    {
        if (day_classes.empty())
        {
            continue;
        }

        for (size_t i = 0; i < day_classes.size(); ++i)
        {
            int overlapped_count = 1;
            for (size_t j = 0; j < day_classes.size(); ++j)
            {
                if (i == j)
                {
                    continue;
                }

                const auto& class_a = day_classes[i];
                const auto& class_b = day_classes[j];
                const bool overlaps = !(class_b.end_time < class_a.start_time || class_a.end_time < class_b.start_time);
                if (overlaps)
                {
                    overlapped_count++;
                }
            }
            const auto class_id = day_classes[i].class_id;
            attendance[class_id] += 1.0 / overlapped_count;
        }
    }

    double penalty = 0.0;
    for (int class_id = 0; class_id < state.size(); ++class_id)
    {
        if (!state.is_attended(class_id))
        {
            continue;
        }

        const int group = state.get_raw_group(class_id);
        const auto session_count = static_cast<int>(problem.get_group(class_id, group).sessions.size());
        const auto absences = session_count - attendance[class_id];
        const double absence_fraq = static_cast<double>(absences) / static_cast<double>(session_count);
        penalty += absence_fraq;
    }
    return penalty;

    // ====== OLD PER WEEK OVERLAP ======
    // const size_t total = problem.class_size();
    // if (total == 0) return 0.0;
    //
    // auto overlaps = [](const solver_models::Class& a, const solver_models::Class& b)
    // {
    //     if (a.day != b.day) return false;
    //     if (!(a.week & b.week).any()) return false;
    //     return !(b.end_time < a.start_time || a.end_time < b.start_time);
    // };
    //
    // int counter = 0;
    //
    // for (int class_id = 0; class_id < static_cast<int>(state.size()); ++class_id)
    // {
    //     if (state.is_unattended(class_id))
    //     {
    //         counter++;
    //         continue;
    //     }
    //     if (!state.is_assigned(class_id))
    //     {
    //         continue;
    //     }
    //     for (int cid = class_id + 1; cid < static_cast<int>(state.size()); ++cid)
    //     {
    //         if (!state.is_attended(cid))
    //         {
    //             continue;
    //         }
    //         const auto& class_a = problem.get_group(class_id, state.get_raw_group(class_id));
    //         const auto& class_b = problem.get_group(cid, state.get_raw_group(cid));
    //         if (overlaps(class_a, class_b))
    //         {
    //             counter++;
    //         }
    //     }
    // }
    // return counter;
}

double MinimizeTotalAbsenceConstraint::evaluate(const TimeTableState& state,
                                                const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool MinimizeTotalAbsenceConstraint::is_satisfied(const TimeTableState& state,
                                                  const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool MinimizeTotalAbsenceConstraint::is_feasible(const TimeTableState& state,
                                                    const TimeTableProblem& problem,
                                                    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- TimeBlockDayConstraint --------------------

double TimeBlockDayConstraint::penalty(const TimeTableState& state,
                                       const TimeTableProblem& problem) const
{
    double p = 0.0;
    for (int class_id = 0; class_id < static_cast<int>(state.size()); ++class_id)
    {
        if (!state.is_attended(class_id))
            continue;
        const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
        if (cls.day != day)
            continue;
        if (cls.start_time < end_time && start_time < cls.end_time)
            p += 1.0;
    }
    return p;
}

double TimeBlockDayConstraint::evaluate(const TimeTableState& state,
                                        const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool TimeBlockDayConstraint::is_satisfied(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool TimeBlockDayConstraint::is_feasible(const TimeTableState& state,
                                         const TimeTableProblem& problem,
                                         const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- TimeBlockDateConstraint --------------------

double TimeBlockDateConstraint::penalty(const TimeTableState& state,
                                        const TimeTableProblem& problem) const
{
    double p = 0.0;
    for (int class_id = 0; class_id < static_cast<int>(state.size()); ++class_id)
    {
        if (!state.is_attended(class_id)) continue;
        const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
        for (const auto& session : cls.sessions)
        {
            if (session.date != date) continue;
            if (session.start_time < end_time && start_time < session.end_time)
            {
                p += 1.0;
                break;
            }
        }
    }
    return p;
}

double TimeBlockDateConstraint::evaluate(const TimeTableState& state,
                                         const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool TimeBlockDateConstraint::is_satisfied(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool TimeBlockDateConstraint::is_feasible(const TimeTableState& state,
                                          const TimeTableProblem& problem,
                                          const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- PreferEdgeClassConstraint --------------------

double PreferEdgeClassConstraint::penalty(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    if (!state.is_attended(class_id)) return 0.0;
    const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));

    int earliest = cls.start_time;
    int latest   = cls.end_time;
    for (int other_id = 0; other_id < static_cast<int>(state.size()); ++other_id)
    {
        if (!state.is_attended(other_id)) continue;
        const auto& other = problem.get_group(other_id, state.get_raw_group(other_id));
        if (other.day != cls.day) continue;
        earliest = std::min(earliest, other.start_time);
        latest   = std::max(latest,   other.end_time);
    }

    if (position == EdgePosition::Start && cls.start_time == earliest) return 0.0;
    if (position == EdgePosition::End   && cls.end_time   == latest)   return 0.0;
    return 1.0;
}

double PreferEdgeClassConstraint::evaluate(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool PreferEdgeClassConstraint::is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool PreferEdgeClassConstraint::is_feasible(const TimeTableState& state,
                                            const TimeTableProblem& problem,
                                            const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- PreferEdgeGroupConstraint --------------------

double PreferEdgeGroupConstraint::penalty(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    if (!state.is_attended(class_id, group)) return 0.0;
    const auto& cls = problem.get_group(class_id, group);

    int earliest = cls.start_time;
    int latest   = cls.end_time;
    for (int other_id = 0; other_id < static_cast<int>(state.size()); ++other_id)
    {
        if (!state.is_attended(other_id)) continue;
        const auto& other = problem.get_group(other_id, state.get_raw_group(other_id));
        if (other.day != cls.day) continue;
        earliest = std::min(earliest, other.start_time);
        latest   = std::max(latest,   other.end_time);
    }

    if (position == EdgePosition::Start && cls.start_time == earliest) return 0.0;
    if (position == EdgePosition::End   && cls.end_time   == latest)   return 0.0;
    return 1.0;
}

double PreferEdgeGroupConstraint::evaluate(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

bool PreferEdgeGroupConstraint::is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

bool PreferEdgeGroupConstraint::is_feasible(const TimeTableState& state,
                                            const TimeTableProblem& problem,
                                            const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- PUBLIC API --------------------

namespace constraints {

double evaluate_all(const TimeTableProblem& problem, const TimeTableState& state)
{
    double total = 0.0;
    for (const auto& constraint : problem.get_constraints())
    {
        std::visit([&](const auto& c){ total += c.evaluate(state, problem); }, constraint);
    }
    return total;
}

double evaluate_all(const std::span<const ConstraintVariant>& constraints,const TimeTableProblem& problem, const TimeTableState& state)
{
    double total = 0.0;
    for (const auto& constraint : constraints)
    {
        std::visit([&](const auto& c){ total += c.evaluate(state, problem); }, constraint);
    }
    return total;
}

bool are_satisfied(const std::span<const ConstraintVariant>& constraints,const TimeTableProblem& problem, const TimeTableState& state)
{
    for (const auto& constraint : constraints)
    {
        const bool is_satisfied = std::visit([&](const auto& c){ return c.is_satisfied(state, problem); }, constraint);
        if (!is_satisfied)
        {
            return false;
        }
    }
    return true;
}

bool are_feasible(
    const std::span<const ConstraintVariant>& constraints,
    const TimeTableProblem& problem,
    const TimeTableState& state,
    const SequenceContext& context)
{
    for (const auto& constraint : constraints)
    {
        const bool is_feasible = std::visit([&](const auto& c){ return c.is_feasible(state, problem, context); }, constraint);
        if (!is_feasible)
        {
            return false;
        }
    }
    return true;
}

} // namespace constraints
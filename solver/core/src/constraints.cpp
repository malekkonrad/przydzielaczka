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

template<>
double MinimizeGapsConstraint<false>::penalty(
    const TimeTableState& state,
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
}

template<>
double MinimizeGapsConstraint<true>::penalty(
        const TimeTableState& state,
        const TimeTableProblem& problem) const
{
    std::map<std::pair<int,int>, std::vector<const Class*>> by_day_week;
    for (int class_id = 0; class_id < state.size(); ++class_id)
    {
        if (!state.is_attended(class_id))
        {
            continue;
        }
        const auto& cls = problem.get_group(class_id, state.get_raw_group(class_id));
        if (cls.week.test(0))
        {
            by_day_week[{cls.day, 0}].push_back(&cls);
        }
        if (cls.week.test(1))
        {
            by_day_week[{cls.day, 1}].push_back(&cls);
        }
    }

    double penalty = 0.0;
    for (auto& day_classes : by_day_week | std::views::values)
    {
        std::sort(day_classes.begin(), day_classes.end(),
            [](const Class* a, const Class* b)
            {
                if (a->start_time == b->start_time)
                {
                    return a->end_time < b->end_time;
                }
                return a->start_time < b->start_time;
            });

        for (size_t i = 1; i < day_classes.size(); ++i)
        {
            const int gap = day_classes[i]->start_time - day_classes[i - 1]->end_time;
            if (gap > min_break)
            {
                penalty += static_cast<double>(gap - min_break);
            }
        }
    }
    return penalty;
}

template<bool Simplified>
double MinimizeGapsConstraint<Simplified>::evaluate(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool MinimizeGapsConstraint<Simplified>::is_satisfied(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool MinimizeGapsConstraint<Simplified>::is_feasible(
    const TimeTableState& state,
    const TimeTableProblem& problem,
    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- GroupPreferenceConstraint --------------------

template<bool Simplified>
double GroupPreferenceConstraint<Simplified>::penalty(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id, group) ? 0.0 : 1.0;
}

template<bool Simplified>
double GroupPreferenceConstraint<Simplified>::evaluate(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool GroupPreferenceConstraint<Simplified>::is_satisfied(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool GroupPreferenceConstraint<Simplified>::is_feasible(
    const TimeTableState& state,
    const TimeTableProblem& problem,
    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- LecturerPreferenceConstraint --------------------

template<bool Simplified>
double LecturerPreferenceConstraint<Simplified>::penalty(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    if (!state.is_attended(class_id)) return 1.0;
    const int group = state.get_raw_group(class_id);
    return problem.get_group(class_id, group).lecturer == lecturer ? 0.0 : 1.0;
}

template<bool Simplified>
double LecturerPreferenceConstraint<Simplified>::evaluate(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool LecturerPreferenceConstraint<Simplified>::is_satisfied(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool LecturerPreferenceConstraint<Simplified>::is_feasible(
    const TimeTableState& state,
   const TimeTableProblem& problem,
   const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeClassAbsenceConstraint --------------------

template<bool Simplified>
double MinimizeClassAbsenceConstraint<Simplified>::penalty(
    const TimeTableState& state,
    const TimeTableProblem& /*problem*/) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id) ? 0.0 : 1.0;
}

template<bool Simplified>
double MinimizeClassAbsenceConstraint<Simplified>::evaluate(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool MinimizeClassAbsenceConstraint<Simplified>::is_satisfied(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool MinimizeClassAbsenceConstraint<Simplified>::is_feasible(
    const TimeTableState& state,
    const TimeTableProblem& problem,
    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeGroupAbsenceConstraint --------------------

template<bool Simplified>
double MinimizeGroupAbsenceConstraint<Simplified>::penalty(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    if (!state.is_assigned(class_id)) return 0.0;
    return state.is_attended(class_id, group) ? 0.0 : 1.0;
}

template<bool Simplified>
double MinimizeGroupAbsenceConstraint<Simplified>::evaluate(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool MinimizeGroupAbsenceConstraint<Simplified>::is_satisfied(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool MinimizeGroupAbsenceConstraint<Simplified>::is_feasible(
    const TimeTableState& state,
    const TimeTableProblem& problem,
    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- MinimizeTotalAbsenceConstraint --------------------

template<>
double MinimizeTotalAbsenceConstraint<false>::penalty(
    const TimeTableState& state,
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
        const double absence_frac = static_cast<double>(absences) / static_cast<double>(session_count);
        penalty += absence_frac;
    }
    return penalty;
}

template<>
double MinimizeTotalAbsenceConstraint<true>::penalty(
    const TimeTableState& state,
    const TimeTableProblem& problem) const
{
    const size_t total = problem.class_size();
    if (total == 0) return 0.0;

    auto overlaps = [](const solver_models::Class& a, const solver_models::Class& b)
    {
        if (a.day != b.day) return false;
        if (!(a.week & b.week).any()) return false;
        return !(b.end_time < a.start_time || a.end_time < b.start_time);
    };

    int counter = 0;

    for (int class_id = 0; class_id < static_cast<int>(state.size()); ++class_id)
    {
        if (state.is_unattended(class_id))
        {
            counter++;
            continue;
        }
        if (!state.is_assigned(class_id))
        {
            continue;
        }
        for (int cid = class_id + 1; cid < static_cast<int>(state.size()); ++cid)
        {
            if (!state.is_attended(cid))
            {
                continue;
            }
            const auto& class_a = problem.get_group(class_id, state.get_raw_group(class_id));
            const auto& class_b = problem.get_group(cid, state.get_raw_group(cid));
            if (overlaps(class_a, class_b))
            {
                counter++;
            }
        }
    }
    return counter;
}

template<bool Simplified>
double MinimizeTotalAbsenceConstraint<Simplified>::evaluate(const TimeTableState& state,
                                                const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool MinimizeTotalAbsenceConstraint<Simplified>::is_satisfied(const TimeTableState& state,
                                                  const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool MinimizeTotalAbsenceConstraint<Simplified>::is_feasible(const TimeTableState& state,
                                                    const TimeTableProblem& problem,
                                                    const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// TODO add per session evaluation
// -------------------- TimeBlockDayConstraint --------------------

template<bool Simplified>
double TimeBlockDayConstraint<Simplified>::penalty(const TimeTableState& state,
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

template<bool Simplified>
double TimeBlockDayConstraint<Simplified>::evaluate(const TimeTableState& state,
                                        const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool TimeBlockDayConstraint<Simplified>::is_satisfied(const TimeTableState& state,
                                          const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool TimeBlockDayConstraint<Simplified>::is_feasible(const TimeTableState& state,
                                         const TimeTableProblem& problem,
                                         const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- TimeBlockDateConstraint --------------------

template<bool Simplified>
double TimeBlockDateConstraint<Simplified>::penalty(const TimeTableState& state,
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

template<bool Simplified>
double TimeBlockDateConstraint<Simplified>::evaluate(const TimeTableState& state,
                                         const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool TimeBlockDateConstraint<Simplified>::is_satisfied(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool TimeBlockDateConstraint<Simplified>::is_feasible(const TimeTableState& state,
                                          const TimeTableProblem& problem,
                                          const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- PreferEdgeClassConstraint --------------------

template<bool Simplified>
double PreferEdgeClassConstraint<Simplified>::penalty(const TimeTableState& state,
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

template<bool Simplified>
double PreferEdgeClassConstraint<Simplified>::evaluate(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool PreferEdgeClassConstraint<Simplified>::is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool PreferEdgeClassConstraint<Simplified>::is_feasible(const TimeTableState& state,
                                            const TimeTableProblem& problem,
                                            const SequenceContext& context) const
{
    if (!context.has_score(id)) return true;
    return penalty(state, problem) <= context[id] + slack;
}

// -------------------- PreferEdgeGroupConstraint --------------------

template<bool Simplified>
double PreferEdgeGroupConstraint<Simplified>::penalty(const TimeTableState& state,
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

template<bool Simplified>
double PreferEdgeGroupConstraint<Simplified>::evaluate(const TimeTableState& state,
                                           const TimeTableProblem& problem) const
{
    const double p = penalty(state, problem);
    return weight * p;
}

template<bool Simplified>
bool PreferEdgeGroupConstraint<Simplified>::is_satisfied(const TimeTableState& state,
                                             const TimeTableProblem& problem) const
{
    return penalty(state, problem) == 0.0;
}

template<bool Simplified>
bool PreferEdgeGroupConstraint<Simplified>::is_feasible(const TimeTableState& state,
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

double evaluate_all(const std::span<const ConstraintVariant>& constraints, const TimeTableProblem& problem, const TimeTableState& state)
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

// -------------------- Explicit instantiations --------------------

template struct solver_models::MinimizeGapsConstraint<true>;
template struct solver_models::MinimizeGapsConstraint<false>;
template struct solver_models::GroupPreferenceConstraint<true>;
template struct solver_models::GroupPreferenceConstraint<false>;
template struct solver_models::LecturerPreferenceConstraint<true>;
template struct solver_models::LecturerPreferenceConstraint<false>;
template struct solver_models::TimeBlockDayConstraint<true>;
template struct solver_models::TimeBlockDayConstraint<false>;
template struct solver_models::TimeBlockDateConstraint<true>;
template struct solver_models::TimeBlockDateConstraint<false>;
template struct solver_models::PreferEdgeClassConstraint<true>;
template struct solver_models::PreferEdgeClassConstraint<false>;
template struct solver_models::PreferEdgeGroupConstraint<true>;
template struct solver_models::PreferEdgeGroupConstraint<false>;
template struct solver_models::MinimizeClassAbsenceConstraint<true>;
template struct solver_models::MinimizeClassAbsenceConstraint<false>;
template struct solver_models::MinimizeGroupAbsenceConstraint<true>;
template struct solver_models::MinimizeGroupAbsenceConstraint<false>;
template struct solver_models::MinimizeTotalAbsenceConstraint<true>;
template struct solver_models::MinimizeTotalAbsenceConstraint<false>;
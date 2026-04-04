//
// Created by mateu on 04.04.2026.
//

#pragma once

#include "time_table_state.h"
#include <constraint_variant_fwd.h>

#include <concepts>
#include <time_table_problem.h>
#include <variant>
#include <vector>

class TimeTableProblem;

// -------------------- SEQUENCE CONTEXT --------------------

namespace constraints
{
    // Holds the best penalty score per constraint (indexed by constraint id)
    // observed after solving a sequence. Passed to is_feasible() when solving
    // subsequent sequences to enforce slack-bounded feasibility.
    // Empty best_scores means no previous sequence has been solved yet.
    struct SequenceContext
    {
        std::vector<double> best_scores;

        [[nodiscard]] bool has_score(const int id) const
        {
            return id >= 0 && id < static_cast<int>(best_scores.size());
        }
    };

    double evaluate_all(const TimeTableProblem& problem,
                        const TimeTableState& state);

    double evaluate_all(const std::span<const solver_models::ConstraintVariant>& constraints,
                        const TimeTableProblem& problem,
                        const TimeTableState& state);

    bool are_satisfied(const std::span<const solver_models::ConstraintVariant>& constraints,
                       const TimeTableProblem& problem,
                       const TimeTableState& state);

    bool are_feasible(const std::span<const solver_models::ConstraintVariant>& constraints,
                      const TimeTableProblem& problem,
                      const TimeTableState& state,
                      const SequenceContext& context);

} // namespace constraints


namespace solver_models {

enum class EdgePosition { Start, End };

// -------------------- CONSTRAINT STRUCTS --------------------

struct MinimizeGapsConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int min_break{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct GroupPreferenceConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    int group{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct LecturerPreferenceConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    int lecturer{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct TimeBlockDayConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int start_time{};
    int end_time{};
    int day{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct TimeBlockDateConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int start_time{};
    int end_time{};
    int date{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct PreferEdgeClassConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    EdgePosition position = EdgePosition::Start;
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct PreferEdgeGroupConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    int group{};
    EdgePosition position = EdgePosition::Start;
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct MinimizeClassAbsenceConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct MinimizeGroupAbsenceConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int class_id{};
    int group{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

struct MinimizeTotalAbsenceConstraint
{
    int sequence{};
    double weight{};
    bool hard{};
    int slack{};
    int id = 0;

    [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
    [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const constraints::SequenceContext& context) const;
};

// -------------------- CONCEPT --------------------

// Requires that a type exposes all four evaluation methods.
// TimeTableProblem is forward-declared here; the static_assert in constraints.cpp
// fires with the full definition, so the check is complete at that point.
template<typename T>
concept Evaluatable = requires(
    const T& c,
    const TimeTableState& s,
    const TimeTableProblem& p,
    const constraints::SequenceContext& ctx)
{
    { c.penalty(s, p) }           -> std::convertible_to<double>;
    { c.evaluate(s, p) }          -> std::convertible_to<double>;
    { c.is_satisfied(s, p) }      -> std::convertible_to<bool>;
    { c.is_feasible(s, p, ctx) }  -> std::convertible_to<bool>;
};

namespace detail {
    template<typename Variant>
    struct all_evaluatable_impl : std::false_type {};

    template<typename... Ts>
    struct all_evaluatable_impl<std::variant<Ts...>>
        : std::bool_constant<(Evaluatable<Ts> && ...)> {};
} // namespace detail

// True iff every alternative in Variant satisfies Evaluatable.
template<typename Variant>
inline constexpr bool all_evaluatable_v = detail::all_evaluatable_impl<Variant>::value;

} // namespace solver_models
//
// Created by mateu on 04.04.2026.
//

#pragma once

#include "time_table_state.h"

#include <concepts>
#include <span>
#include <variant>

class TimeTableProblem;
class SequenceContext;

namespace constraints
{
    // -------------------- ENUMS --------------------

    enum class ConstraintType
    {
        Null,
        MinimizeGaps,
        GroupPreference,
        LecturerPreference,
        TimeBlockDay,
        TimeBlockDate,
        PreferEdgeClass,
        PreferEdgeGroup,
        MinimizeClassAbsence,
        MinimizeGroupAbsence,
        MinimizeTotalAbsence,
    };
} // namespace constraints


// solver_models structs + ConstraintVariant must be defined before the
// constraints free functions that reference solver_models::ConstraintVariant.

namespace solver_models {

    enum class EdgePosition { Start, End };

    // -------------------- CONSTRAINT STRUCTS --------------------
    template<bool Simplified = true>
    struct MinimizeGapsConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int min_break{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::MinimizeGaps;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using MinimizeGapsConstraintWeekly = MinimizeGapsConstraint<true>;
    using MinimizeGapsConstraintYearly = MinimizeGapsConstraint<false>;

    template<bool Simplified = true>
    struct GroupPreferenceConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int class_id{};
        int group{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::GroupPreference;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using GroupPreferenceConstraintWeekly = GroupPreferenceConstraint<true>;
    using GroupPreferenceConstraintYearly = GroupPreferenceConstraint<false>;

    template<bool Simplified = true>
    struct LecturerPreferenceConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int class_id{};
        int lecturer{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::LecturerPreference;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using LecturerPreferenceConstraintWeekly = LecturerPreferenceConstraint<true>;
    using LecturerPreferenceConstraintYearly = LecturerPreferenceConstraint<false>;

    template<bool Simplified = true>
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
        constraints::ConstraintType type = constraints::ConstraintType::TimeBlockDay;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using TimeBlockDayConstraintWeekly = TimeBlockDayConstraint<true>;
    using TimeBlockDayConstraintYearly = TimeBlockDayConstraint<false>;

    template<bool Simplified = true>
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
        constraints::ConstraintType type = constraints::ConstraintType::TimeBlockDate;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using TimeBlockDateConstraintWeekly = TimeBlockDateConstraint<true>;
    using TimeBlockDateConstraintYearly = TimeBlockDateConstraint<false>;

    template<bool Simplified = true>
    struct PreferEdgeClassConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int class_id{};
        EdgePosition position = EdgePosition::Start;
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::PreferEdgeClass;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using PreferEdgeClassConstraintWeekly = PreferEdgeClassConstraint<true>;
    using PreferEdgeClassConstraintYearly = PreferEdgeClassConstraint<false>;

    template<bool Simplified = true>
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
        constraints::ConstraintType type = constraints::ConstraintType::PreferEdgeGroup;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using PreferEdgeGroupConstraintWeekly = PreferEdgeGroupConstraint<true>;
    using PreferEdgeGroupConstraintYearly = PreferEdgeGroupConstraint<false>;

    template<bool Simplified = true>
    struct MinimizeClassAbsenceConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int class_id{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::MinimizeClassAbsence;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using MinimizeClassAbsenceConstraintWeekly = MinimizeClassAbsenceConstraint<true>;
    using MinimizeClassAbsenceConstraintYearly = MinimizeClassAbsenceConstraint<false>;

    template<bool Simplified = true>
    struct MinimizeGroupAbsenceConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int class_id{};
        int group{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::MinimizeGroupAbsence;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using MinimizeGroupAbsenceConstraintWeekly = MinimizeGroupAbsenceConstraint<true>;
    using MinimizeGroupAbsenceConstraintYearly = MinimizeGroupAbsenceConstraint<false>;

    template<bool Simplified = true>
    struct MinimizeTotalAbsenceConstraint
    {
        int sequence{};
        double weight{};
        bool hard{};
        int slack{};
        int id = 0;
        constraints::ConstraintType type = constraints::ConstraintType::MinimizeTotalAbsence;

        [[nodiscard]] double penalty(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] double evaluate(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_satisfied(const TimeTableState& state, const TimeTableProblem& problem) const;
        [[nodiscard]] bool   is_feasible(const TimeTableState& state, const TimeTableProblem& problem, const SequenceContext& context) const;
    };

    using MinimizeTotalAbsenceConstraintWeekly = MinimizeTotalAbsenceConstraint<true>;
    using MinimizeTotalAbsenceConstraintYearly = MinimizeTotalAbsenceConstraint<false>;

    using ConstraintVariant = std::variant<
        MinimizeGapsConstraint<true>,
        GroupPreferenceConstraint<true>,
        LecturerPreferenceConstraint<true>,
        TimeBlockDayConstraint<true>,
        TimeBlockDateConstraint<true>,
        PreferEdgeClassConstraint<true>,
        PreferEdgeGroupConstraint<true>,
        MinimizeClassAbsenceConstraint<true>,
        MinimizeGroupAbsenceConstraint<true>,
        MinimizeTotalAbsenceConstraint<true>
    >;

    using YearlyConstraintVariant = std::variant<
        MinimizeGapsConstraint<false>,
        GroupPreferenceConstraint<false>,
        LecturerPreferenceConstraint<false>,
        TimeBlockDayConstraint<false>,
        TimeBlockDateConstraint<false>,
        PreferEdgeClassConstraint<false>,
        PreferEdgeGroupConstraint<false>,
        MinimizeClassAbsenceConstraint<false>,
        MinimizeGroupAbsenceConstraint<false>,
        MinimizeTotalAbsenceConstraint<false>
    >;

    template<bool Simplified>
    using ConstraintVariantFor = std::conditional_t<Simplified,
        ConstraintVariant,
        YearlyConstraintVariant
    >;

    template<typename T>
    struct as_yearly;

    template<template<bool> class C>
    struct as_yearly<C<true>> { using type = C<false>; };

    template<typename T>
    using as_yearly_t = typename as_yearly<T>::type;
} // namespace solver_models


namespace constraints
{
    // -------------------- CONCEPT --------------------

    inline namespace concepts
    {
        // Requires that a type exposes all four evaluation methods.
        // TimeTableProblem is forward-declared here; the static_assert in constraints.cpp
        // fires with the full definition, so the check is complete at that point.
        template<typename T>
        concept Evaluatable = requires(
            const T& c,
            const TimeTableState& s,
            const TimeTableProblem& p,
            const SequenceContext& ctx)
        {
            { c.penalty(s, p) }           -> std::convertible_to<double>;
            { c.evaluate(s, p) }          -> std::convertible_to<double>;
            { c.is_satisfied(s, p) }      -> std::convertible_to<bool>;
            { c.is_feasible(s, p, ctx) }  -> std::convertible_to<bool>;
            { c.id }                      -> std::convertible_to<int>;
            { c.sequence }                -> std::convertible_to<int>;
            { c.hard }                    -> std::convertible_to<bool>;
            { c.type }                    -> std::convertible_to<ConstraintType>;
        };

        namespace detail {
            template<typename Variant>
            struct all_evaluatable_impl : std::false_type {};

            template<typename... Ts>
            struct all_evaluatable_impl<std::variant<Ts...>>
                : std::bool_constant<(Evaluatable<Ts> && ...)> {};
        } // namespace detail

        // True if every alternative in Variant satisfies Evaluatable.
        template<typename Variant>
        inline constexpr bool all_evaluatable_v = detail::all_evaluatable_impl<Variant>::value;
    } // namespace concepts

    // -------------------- FREE FUNCTIONS --------------------

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
//
// Created by mateu on 06.04.2026.
//

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <constraints.h>
#include <time_table_problem.h>
#include <time_table_state.h>

namespace policies {

    // ==================== CONCEPTS ====================

    inline namespace concepts
    {
        // Evaluatable - alias for evaluatable
        template<typename Policy>
        concept Evaluatable = constraints::Evaluatable<Policy>;

        // Substitutable — policy can construct itself from a matching ConstraintVariant.
        // PolicyEvaluator uses this to automatically replace all constraints whose
        // ConstraintType matches P{}.type, copying all fields via the factory method.
        template<typename Policy>
        concept Substitutable = Evaluatable<Policy> && requires(
            const solver_models::ConstraintVariant& c,
            const TimeTableProblem& problem)
        {
            { Policy::make(c, problem) } -> std::same_as<Policy>;
        };

        // PartiallyEvaluatable — optional extension for solver_models::Evaluatable types.
        //
        // Policies that implement this concept can be evaluated against a single
        // (class_id, group) assignment instead of the full state, enabling finer-grained
        // pruning. PolicyConstraintEvaluator<P> checks this at compile time and dispatches
        // to the partial overloads when available, falling back to full evaluation otherwise.
        template<typename Policy>
        concept PartiallyEvaluatable = Evaluatable<Policy> && requires(
            const Policy& p,
            const TimeTableState& state,
            const TimeTableProblem& problem,
            const SequenceContext& ctx,
            int class_id, int group)
        {
            { p.partial_evaluate(state, problem, class_id, group)         } -> std::convertible_to<double>;
            { p.partial_is_satisfied(state, problem, class_id, group)     } -> std::convertible_to<bool>;
            { p.partial_is_feasible(state, problem, ctx, class_id, group) } -> std::convertible_to<bool>;
        };
    } // namespace concepts

    // ==================== POLICY FREE FUNCTIONS ====================

    // Work at the constraint/policy level: take any solver_models::Evaluatable.
    // These are the generic equivalents of evaluate_all / are_satisfied
    // / are_feasible, but work with policies as well as constraints.

    // Evaluate a single constraint or policy.
    template<Evaluatable E>
    double evaluate(const E& e, const TimeTableState& state, const TimeTableProblem& problem)
    {
        return e.evaluate(state, problem);
    }

    // True if a single constraint or policy is satisfied.
    template<Evaluatable E>
    bool is_satisfied(const E& e, const TimeTableState& state, const TimeTableProblem& problem)
    {
        return e.is_satisfied(state, problem);
    }

    // True if a single constraint or policy is feasible given the sequence context.
    template<Evaluatable E>
    bool is_feasible(const E& e, const TimeTableState& state, const TimeTableProblem& problem,
                     const SequenceContext& context)
    {
        return e.is_feasible(state, problem, context);
    }

    // Sum evaluate() over a homogeneous range of constraints or policies.
    template<std::ranges::input_range R>
        requires Evaluatable<std::ranges::range_value_t<R>>
    double evaluate_all(const R& range, const TimeTableState& state, const TimeTableProblem& problem)
    {
        double total = 0.0;
        for (const auto& e : range)
            total += e.evaluate(state, problem);
        return total;
    }

    // True if every element in the range is satisfied.
    template<std::ranges::input_range R>
        requires Evaluatable<std::ranges::range_value_t<R>>
    bool all_satisfied(const R& range, const TimeTableState& state, const TimeTableProblem& problem)
    {
        return std::ranges::all_of(range, [&](const auto& e)
        {
            return e.is_satisfied(state, problem);
        });
    }

    // True if every element in the range is feasible.
    template<std::ranges::input_range R>
        requires Evaluatable<std::ranges::range_value_t<R>>
    bool all_feasible(const R& range, const TimeTableState& state, const TimeTableProblem& problem,
                      const SequenceContext& context)
    {
        return std::ranges::all_of(range, [&](const auto& e)
        {
            return e.is_feasible(state, problem, context);
        });
    }

    // ==================== NULL POLICY ====================

    // No-op policy — always satisfied, zero penalty.
    // Used as the default template argument for PolicyEvaluator.
    struct NullPolicy
    {
        int id       = -1;
        int sequence = -1;
        bool hard    = false;
        constraints::ConstraintType type = constraints::ConstraintType::Null;

        static NullPolicy make(const solver_models::ConstraintVariant&, const TimeTableProblem&) { return {}; }

        [[nodiscard]] double penalty(const TimeTableState&, const TimeTableProblem&) const { return 0.0; }
        [[nodiscard]] double evaluate(const TimeTableState&, const TimeTableProblem&) const { return 0.0; }
        [[nodiscard]] bool   is_satisfied(const TimeTableState&, const TimeTableProblem&) const { return true; }
        [[nodiscard]] bool   is_feasible(const TimeTableState&, const TimeTableProblem&, const SequenceContext&) const { return true; }
    };

} // namespace policies
//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <span>
#include <variant>
#include <vector>
#include <constraint_variant_fwd.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <policies/policies.h>
#include <sequence_context.h>
#include <traits.h>

// ==================== CONCEPTS ====================

// SequenceEvaluator — anything that can score and prune a TimeTableState.
//
// The active sequence is set once per backtracking pass via set_sequence().
// All evaluation methods then operate on that stored sequence.

namespace evaluator
{
    inline namespace concepts
    {
        template<typename E>
        concept SequenceEvaluator = requires(
            E e,
            const TimeTableState& state,
            SequenceContext& ctx,
            const SequenceContext& cctx,
            int sequence)
        {
            { e.set_sequence(sequence)     } -> std::same_as<void>;
            { e.score(state)               } -> std::same_as<SequenceContext>;
            { e.update_context(ctx, state) } -> std::same_as<void>;
            { e.evaluate(state)            } -> std::convertible_to<double>;
            { e.are_satisfied(state)       } -> std::convertible_to<bool>;
            { e.are_feasible(state, cctx)  } -> std::convertible_to<bool>;
        };

        // PartialSequenceEvaluator — evaluator-level extension of SequenceEvaluator.
        //
        // Satisfied by BasicConstraintEvaluator<Traits> whenever
        // Traits::config.use_partial_evaluation is true (the partial_* methods exist
        // regardless, but solvers use this concept at compile time to dispatch to the
        // cheaper per-(class_id, group) pruning path).
        template<typename E>
        concept PartialSequenceEvaluator = SequenceEvaluator<E> && requires(
            E e,
            const TimeTableState& state,
            const SequenceContext& ctx,
            int class_id,
            int group)
        {
            { e.partial_evaluate(state, class_id, group)          } -> std::convertible_to<double>;
            { e.partial_are_satisfied(state, class_id, group)     } -> std::convertible_to<bool>;
            { e.partial_are_feasible(state, ctx, class_id, group) } -> std::convertible_to<bool>;
        };
    } // namespace concepts

    namespace detail {
        // Appends a single type T to the alternatives of a std::variant.
        template<typename V, typename T>
        struct variant_append;

        template<typename... Ts, typename T>
        struct variant_append<std::variant<Ts...>, T> { using type = std::variant<Ts..., T>; };

        // Appends zero or more types to a variant (variadic fold).
        template<typename V, typename... Ts>
        struct variant_append_all { using type = V; };

        template<typename V, typename T, typename... Ts>
        struct variant_append_all<V, T, Ts...>
            : variant_append_all<typename variant_append<V, T>::type, Ts...> {};

        template<typename V, typename... Ts>
        using variant_append_all_t = typename variant_append_all<V, Ts...>::type;

        // True iff T appears in Ts...
        template<typename T, typename... Ts>
        inline constexpr bool contains_type_v = (std::is_same_v<T, Ts> || ...);

        // True iff all types in Ts... are distinct.
        template<typename... Ts>
        inline constexpr bool all_unique_types_v = true;

        template<typename T, typename... Ts>
        inline constexpr bool all_unique_types_v<T, Ts...> =
            !contains_type_v<T, Ts...> && all_unique_types_v<Ts...>;
    } // namespace detail

} // namespace evaluator

// ==================== CONSTRAINT EVALUATOR ====================

// Forward declaration — body lives in the partial specialization below.
template<typename Traits = SolverTraits>
class BasicConstraintEvaluator;

// Partial specialization unpacking BasicSolverTraits<Config, Ps...>.
//
// Config.use_partial_evaluation — when true the partial_* methods dispatch to
//   dedicated per-(class_id, group) overloads for items satisfying
//   policies::PartiallyEvaluatable; otherwise they fall back to full-state eval.
//
// Ps... — zero or more Substitutable policy types.  For every constraint whose
//   ConstraintType matches P{}.type, P::make(c, problem) replaces the original.
//   Unmatched constraints are stored as-is.
//
// Construction: only the TimeTableProblem is passed; policies come from the type.
//
//   BasicConstraintEvaluator<>
//   BasicConstraintEvaluator<SolverTraits::WithPolicies<IntTimePolicy>>
//   BasicConstraintEvaluator<SolverTraits
//       ::WithPartialEvaluation<true>
//       ::WithPolicies<IntTimePolicy, IntAbsencePolicy>>
//
template<detail::SolverConfig Config, policies::Substitutable... Ps>
class BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>
{
    static_assert(evaluator::detail::all_unique_types_v<Ps...>,
        "Each policy type must appear at most once in Traits::WithPolicies<...>");

public:
    // All constraint alternatives + every policy type in one flat variant.
    using UnifiedVariant = evaluator::detail::variant_append_all_t<solver_models::ConstraintVariant, Ps...>;

    explicit BasicConstraintEvaluator(const TimeTableProblem& problem)
        : problem_(problem), sequence_(0)
    {
        for (const auto& c : problem.get_constraints())
        {
            const auto ctype = std::visit([](const auto& cv) { return cv.type; }, c);

            bool substituted = false;
            ([&]<typename P>(std::type_identity<P>)
            {
                if (!substituted && P{}.type == ctype)
                {
                    unified_.push_back(P::make(c, problem));
                    substituted = true;
                }
            }(std::type_identity<Ps>{}), ...);

            if (!substituted)
            {
                unified_.push_back(std::visit([](const auto& cv) -> UnifiedVariant { return cv; }, c));
            }
        }

        // Sort: ascending sequence, hard before soft within a sequence.
        std::stable_sort(unified_.begin(), unified_.end(),
            [](const UnifiedVariant& a, const UnifiedVariant& b)
            {
                const int  sa = std::visit([](const auto& x) { return x.sequence; }, a);
                const int  sb = std::visit([](const auto& x) { return x.sequence; }, b);
                if (sa != sb) return sa < sb;
                const bool ha = std::visit([](const auto& x) { return x.hard; }, a);
                const bool hb = std::visit([](const auto& x) { return x.hard; }, b);
                return ha && !hb;
            });

        // Build split points (same algorithm as TimeTableProblem).
        if (!unified_.empty())
        {
            const int max_seq = std::visit([](const auto& x) { return x.sequence; }, unified_.back());
            soft_hard_split_.assign(max_seq + 1, static_cast<int>(unified_.size()));
            split_point_.resize(max_seq + 1);

            for (int i = 0; i < static_cast<int>(unified_.size()); ++i)
            {
                const int  seq     = std::visit([](const auto& x) { return x.sequence; }, unified_[i]);
                const bool is_hard = std::visit([](const auto& x) { return x.hard; },     unified_[i]);
                split_point_[seq] = i + 1;
                if (!is_hard && soft_hard_split_[seq] == static_cast<int>(unified_.size()))
                    soft_hard_split_[seq] = i;
            }
            for (int i = 0; i < static_cast<int>(split_point_.size()); ++i)
            {
                if (soft_hard_split_[i] == static_cast<int>(unified_.size()))
                    soft_hard_split_[i] = split_point_[i];
            }
        }
    }

    void set_sequence(const int sequence)   { sequence_ = sequence; }
    [[nodiscard]] int  get_sequence() const { return sequence_; }

    [[nodiscard]] SequenceContext score(const TimeTableState& state) const;
    void update_context(SequenceContext& context, const TimeTableState& state) const;
    [[nodiscard]] double evaluate(const TimeTableState& state) const;
    [[nodiscard]] bool   are_satisfied(const TimeTableState& state) const;
    [[nodiscard]] bool   are_feasible(const TimeTableState& state, const SequenceContext& context) const;

    // B&B support — operates on the current sequence's soft goals.

    // Sums lower_bound() for every BoundEstimating goal in this sequence.
    // Non-BoundEstimating goals contribute 0 (optimistic: no remaining penalty).
    [[nodiscard]] double lower_bound(const TimeTableState& state) const;

    // Returns the number of OrderSensitive policies active in the current sequence.
    [[nodiscard]] int count_order_sensitive() const;

    // Returns class_order() from the first OrderSensitive policy in the current sequence.
    [[nodiscard]] std::vector<std::pair<int,int>> get_class_group_order() const;

    // Partial evaluation — operates on the (class_id, group) slot just placed.
    // When Config.use_partial_evaluation is true and the item satisfies
    // PartiallyEvaluatable, dedicated partial overloads are used; otherwise falls
    // back to full-state evaluation.
    [[nodiscard]] double partial_evaluate(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_satisfied(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_feasible(const TimeTableState& state, const SequenceContext& context, int class_id, int group) const;

private:
    const TimeTableProblem& problem_;
    std::vector<UnifiedVariant> unified_;
    std::vector<int> split_point_;      // split_point_[seq]     = one-past-end of seq's entries
    std::vector<int> soft_hard_split_;  // soft_hard_split_[seq] = first soft entry of seq
    int sequence_;

    // All (hard+soft) entries for exactly seq.
    [[nodiscard]] std::span<const UnifiedVariant> slice_in(const int seq) const
    {
        if (seq >= static_cast<int>(split_point_.size())) return unified_;
        const size_t start = seq < 1 ? 0 : static_cast<size_t>(split_point_[seq - 1]);
        const size_t end   = static_cast<size_t>(split_point_[seq]);
        return std::span(unified_).subspan(start, end - start);
    }

    // Hard-only entries for exactly seq.
    [[nodiscard]] std::span<const UnifiedVariant> slice_hard(const int seq) const
    {
        if (seq >= static_cast<int>(split_point_.size())) return unified_;
        const size_t start = seq < 1 ? 0 : static_cast<size_t>(split_point_[seq - 1]);
        const size_t end   = static_cast<size_t>(soft_hard_split_[seq]);
        return std::span(unified_).subspan(start, end - start);
    }

    // Soft-only entries for exactly seq.
    [[nodiscard]] std::span<const UnifiedVariant> slice_goals(const int seq) const
    {
        if (seq >= static_cast<int>(split_point_.size())) return {};
        const size_t start = static_cast<size_t>(soft_hard_split_[seq]);
        const size_t end   = static_cast<size_t>(split_point_[seq]);
        return std::span(unified_).subspan(start, end - start);
    }

    // All entries with sequence <= seq.
    [[nodiscard]] std::span<const UnifiedVariant> slice_up_to(const int seq) const
    {
        if (seq >= static_cast<int>(split_point_.size())) return unified_;
        return std::span(unified_).first(static_cast<size_t>(split_point_[seq]));
    }

    // All entries with sequence < seq.
    [[nodiscard]] std::span<const UnifiedVariant> slice_before(const int seq) const
    {
        if (seq >= static_cast<int>(split_point_.size())) return unified_;
        const size_t end = seq < 1 ? 0 : static_cast<size_t>(split_point_[seq - 1]);
        return std::span(unified_).first(end);
    }
};

// -------------------- Inline implementations --------------------

template<detail::SolverConfig Config, policies::Substitutable... Ps>
SequenceContext BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::score(
    const TimeTableState& state) const
{
    SequenceContext context(problem_.get_constraints().size());
    for (const auto& u : slice_up_to(sequence_))
        std::visit([&](const auto& x){ context[x.id] = x.penalty(state, problem_); }, u);
    return context;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
void BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::update_context(
    SequenceContext& context, const TimeTableState& state) const
{
    for (const auto& u : slice_in(sequence_))
        std::visit([&](const auto& x)
        {
            if (context[x.id] != 0.0)
            {
                const double pen = x.penalty(state, problem_);
                if (pen < context[x.id]) context[x.id] = pen;
            }
        }, u);
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
double BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::evaluate(
    const TimeTableState& state) const
{
    double total = 0.0;
    for (const auto& u : slice_goals(sequence_))
        std::visit([&](const auto& x){ total += x.evaluate(state, problem_); }, u);
    return total;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
bool BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::are_satisfied(
    const TimeTableState& state) const
{
    for (const auto& u : slice_hard(sequence_))
    {
        const bool ok = std::visit([&](const auto& x)
            { return x.is_satisfied(state, problem_); }, u);
        if (!ok) return false;
    }
    return true;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
bool BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::are_feasible(
    const TimeTableState& state, const SequenceContext& context) const
{
    for (const auto& u : slice_before(sequence_))
    {
        const bool ok = std::visit([&](const auto& x)
            { return x.is_feasible(state, problem_, context); }, u);
        if (!ok) return false;
    }
    return true;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
double BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::partial_evaluate(
    const TimeTableState& state, const int class_id, const int group) const
{
    double total = 0.0;
    for (const auto& u : slice_goals(sequence_))
        std::visit([&](const auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (Config.use_partial_evaluation && policies::PartiallyEvaluatable<T>)
                total += x.partial_evaluate(state, problem_, class_id, group);
            else
                total += x.evaluate(state, problem_);
        }, u);
    return total;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
bool BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::partial_are_satisfied(
    const TimeTableState& state, const int class_id, const int group) const
{
    for (const auto& u : slice_hard(sequence_))
    {
        const bool ok = std::visit([&](const auto& x) -> bool
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (Config.use_partial_evaluation && policies::PartiallyEvaluatable<T>)
                return x.partial_is_satisfied(state, problem_, class_id, group);
            else
                return x.is_satisfied(state, problem_);
        }, u);
        if (!ok) return false;
    }
    return true;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
bool BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::partial_are_feasible(
    const TimeTableState& state, const SequenceContext& context,
    const int class_id, const int group) const
{
    for (const auto& u : slice_before(sequence_))
    {
        const bool ok = std::visit([&](const auto& x) -> bool
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (Config.use_partial_evaluation && policies::PartiallyEvaluatable<T>)
                return x.partial_is_feasible(state, problem_, context, class_id, group);
            else
                return x.is_feasible(state, problem_, context);
        }, u);
        if (!ok) return false;
    }
    return true;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
double BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::lower_bound(
    const TimeTableState& state) const
{
    double bound = 0.0;
    for (const auto& u : slice_goals(sequence_))
        std::visit([&](const auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::BoundEstimating<T>)
            {
                bound += x.lower_bound(state, problem_);
            }
            // else: not BoundEstimating — optimistically assume 0 remaining penalty
        }, u);
    return bound;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
int BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::count_order_sensitive() const
{
    int count = 0;
    for (const auto& u : slice_in(sequence_))
        std::visit([&](const auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::OrderSensitive<T>)
                ++count;
        }, u);
    return count;
}

template<detail::SolverConfig Config, policies::Substitutable... Ps>
std::vector<std::pair<int,int>>
BasicConstraintEvaluator<BasicSolverTraits<Config, Ps...>>::get_class_group_order() const
{
    for (const auto& u : slice_in(sequence_))
    {
        std::vector<std::pair<int,int>> result;
        bool found = false;
        std::visit([&](const auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::OrderSensitive<T>)
            {
                result = x.class_order(problem_);
                found  = true;
            }
        }, u);
        if (found) return result;
    }
    return {};
}

// Default alias — no policies, no partial evaluation.
using ConstraintEvaluator = BasicConstraintEvaluator<>;

static_assert(evaluator::SequenceEvaluator<BasicConstraintEvaluator<>>,
    "BasicConstraintEvaluator<> must satisfy SequenceEvaluator");

// ==================== EVALUATOR FREE FUNCTIONS ====================
// Work at the evaluator level (sequence-aware): take any SequenceEvaluator.

namespace evaluator {

// Sums the soft-goal score across all sequences.
template<concepts::SequenceEvaluator E>
double evaluate_all(E& ev, const TimeTableState& state, const int n_sequences)
{
    double total = 0.0;
    for (int seq = 0; seq < n_sequences; ++seq)
    {
        ev.set_sequence(seq);
        total += ev.evaluate(state);
    }
    return total;
}

// Returns true if all hard constraints are satisfied across all sequences.
template<concepts::SequenceEvaluator E>
bool all_satisfied(E& ev, const TimeTableState& state, const int n_sequences)
{
    for (int seq = 0; seq < n_sequences; ++seq)
    {
        ev.set_sequence(seq);
        if (!ev.are_satisfied(state))
            return false;
    }
    return true;
}

} // namespace evaluator

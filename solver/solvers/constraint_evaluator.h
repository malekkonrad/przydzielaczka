//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <algorithm>
#include <concepts>
#include <ranges>
#include <unordered_set>
#include <variant>
#include <vector>
#include <constraint_variant_fwd.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <policies/policies.h>
#include <sequence_context.h>

// ==================== CONCEPTS ====================

// Evaluatable — anything that can score and prune a TimeTableState.
//
// The active sequence is set once per backtracking pass via set_sequence().
// All five evaluation methods then operate on that stored sequence.
// Both ConstraintEvaluator and PolicyConstraintEvaluator<P> satisfy this.

namespace concepts
{
    template<typename E>
    concept ConstraintEvaluator = requires(
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

    // PartialEvaluator — evaluator-level extension of Evaluator.
    //
    // Satisfied by PolicyConstraintEvaluator<P> (always, regardless of P), but not
    // by the plain ConstraintEvaluator.  Solvers check this at compile time with
    // if constexpr to dispatch to the cheaper per-(class_id, group) pruning path.
    template<typename E>
    concept PartialConstraintEvaluator = ConstraintEvaluator<E> && requires(
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

// ==================== BASELINE EVALUATOR ====================

// Non-template evaluator — delegates entirely to the constraint objects.
class BaseEvaluator
{
public:
    explicit BaseEvaluator(const TimeTableProblem& problem)
        : problem_(problem), sequence_(0) {}

    void set_sequence(const int sequence) { sequence_ = sequence; }
    int  get_sequence() const             { return sequence_; }

    [[nodiscard]] SequenceContext score(const TimeTableState& state) const;
    void update_context(SequenceContext& context, const TimeTableState& state) const;
    [[nodiscard]] double evaluate(const TimeTableState& state) const;
    [[nodiscard]] bool   are_satisfied(const TimeTableState& state) const;
    [[nodiscard]] bool   are_feasible(const TimeTableState& state, const SequenceContext& context) const;

private:
    const TimeTableProblem& problem_;
    int sequence_;
};

static_assert(concepts::ConstraintEvaluator<BaseEvaluator>,
    "ConstraintEvaluator must satisfy Evaluatable");

// ==================== POLICY EVALUATOR ====================

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
} // namespace detail

// Template evaluator that combines any number of policy types with the remaining
// problem constraints in a single sorted vector (UnifiedVariant), indexed by
// pre-computed split points. This gives O(1) sequence slicing — no filter scans.
//
// Each Ps must satisfy policies::Evaluatable.
// If a policy type also satisfies policies::PartiallyEvaluatable, partial_*
// methods use the cheaper per-(class_id, group) overloads for items of that type.
//
// Construction: pass one std::vector<Pi> per policy type.
//   PolicyEvaluator<>          ev(problem);               // no policies
//   PolicyEvaluator<A>         ev(problem, {a1, a2});     // one policy type
//   PolicyEvaluator<A, B>      ev(problem, {a1}, {b1});   // two policy types
//
// Each policy replaces the constraint whose id matches policy.id.
// Remaining constraints + all policies are merged, sorted (ascending sequence,
// hard before soft), and indexed identically to TimeTableProblem.
template<policies::Evaluatable... Ps>
class PolicyEvaluator
{
public:
    // All constraint alternatives + every policy type in one flat variant.
    using UnifiedVariant = detail::variant_append_all_t<solver_models::ConstraintVariant, Ps...>;

    explicit PolicyEvaluator(const TimeTableProblem& problem, std::vector<Ps>... policy_lists)
        : problem_(problem), sequence_(0)
    {
        std::unordered_set<int> claimed;

        // Collect claimed ids from every policy list.
        (..., (std::for_each(policy_lists.begin(), policy_lists.end(),
            [&](const auto& p){ claimed.insert(p.id); })));

        // Unclaimed constraints — convert ConstraintVariant → UnifiedVariant.
        for (const auto& c : problem.get_constraints())
        {
            const int id = std::visit([](const auto& cv) { return cv.id; }, c);
            if (!claimed.contains(id))
                unified_.push_back(std::visit([](const auto& cv) -> UnifiedVariant { return cv; }, c));
        }

        // Policies — move each list's items in.
        (..., (std::for_each(std::make_move_iterator(policy_lists.begin()),
            std::make_move_iterator(policy_lists.end()),
            [&](auto&& p){ unified_.push_back(std::move(p)); })));

        // Sort: ascending sequence, hard before soft within sequence.
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

    void set_sequence(const int sequence) { sequence_ = sequence; }
    int  get_sequence() const             { return sequence_; }

    [[nodiscard]] SequenceContext score(const TimeTableState& state) const;
    void update_context(SequenceContext& context, const TimeTableState& state) const;
    [[nodiscard]] double evaluate(const TimeTableState& state) const;
    [[nodiscard]] bool   are_satisfied(const TimeTableState& state) const;
    [[nodiscard]] bool   are_feasible(const TimeTableState& state, const SequenceContext& context) const;

    // Partial evaluation — operates on the (class_id, group) slot just placed in state.
    // For policy items that satisfy PartiallyEvaluatable, dedicated partial overloads are
    // called; all other items fall back to full-state evaluation.
    [[nodiscard]] double partial_evaluate(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_satisfied(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_feasible(const TimeTableState& state, const SequenceContext& context, int class_id, int group) const;

private:
    const TimeTableProblem& problem_;
    std::vector<UnifiedVariant> unified_;
    std::vector<int> split_point_;      // split_point_[seq]      = one-past-end of seq's entries
    std::vector<int> soft_hard_split_;  // soft_hard_split_[seq]  = first soft entry of seq
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

// Fills context with penalties for every entry up to and including sequence.
template<policies::Evaluatable... Ps>
SequenceContext PolicyEvaluator<Ps...>::score(
    const TimeTableState& state) const
{
    SequenceContext context(problem_.get_constraints().size());
    for (const auto& u : slice_up_to(sequence_))
        std::visit([&](const auto& x){ context[x.id] = x.penalty(state, problem_); }, u);
    return context;
}

// Updates context with lower penalties found in this sequence (hard + soft).
template<policies::Evaluatable... Ps>
void PolicyEvaluator<Ps...>::update_context(
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

// Sums soft goals for this sequence.
template<policies::Evaluatable... Ps>
double PolicyEvaluator<Ps...>::evaluate(
    const TimeTableState& state) const
{
    double total = 0.0;
    for (const auto& u : slice_goals(sequence_))
        std::visit([&](const auto& x){ total += x.evaluate(state, problem_); }, u);
    return total;
}

// True if all hard entries in this sequence are satisfied.
template<policies::Evaluatable... Ps>
bool PolicyEvaluator<Ps...>::are_satisfied(
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

// True if all entries from previous sequences are still feasible.
template<policies::Evaluatable... Ps>
bool PolicyEvaluator<Ps...>::are_feasible(
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

// Sums soft goals; items satisfying PartiallyEvaluatable use the partial overload.
template<policies::Evaluatable... Ps>
double PolicyEvaluator<Ps...>::partial_evaluate(
    const TimeTableState& state, const int class_id, const int group) const
{
    double total = 0.0;
    for (const auto& u : slice_goals(sequence_))
        std::visit([&](const auto& x)
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::PartiallyEvaluatable<T>)
                total += x.partial_evaluate(state, problem_, class_id, group);
            else
                total += x.evaluate(state, problem_);
        }, u);
    return total;
}

// True if all hard entries are satisfied; partial overloads used where available.
template<policies::Evaluatable... Ps>
bool PolicyEvaluator<Ps...>::partial_are_satisfied(
    const TimeTableState& state, const int class_id, const int group) const
{
    for (const auto& u : slice_hard(sequence_))
    {
        const bool ok = std::visit([&](const auto& x) -> bool
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::PartiallyEvaluatable<T>)
                return x.partial_is_satisfied(state, problem_, class_id, group);
            else
                return x.is_satisfied(state, problem_);
        }, u);
        if (!ok) return false;
    }
    return true;
}

// True if all previous-sequence entries are feasible; partial overloads used where available.
template<policies::Evaluatable... Ps>
bool PolicyEvaluator<Ps...>::partial_are_feasible(
    const TimeTableState& state, const SequenceContext& context,
    const int class_id, const int group) const
{
    for (const auto& u : slice_before(sequence_))
    {
        const bool ok = std::visit([&](const auto& x) -> bool
        {
            using T = std::decay_t<decltype(x)>;
            if constexpr (policies::PartiallyEvaluatable<T>)
                return x.partial_is_feasible(state, problem_, context, class_id, group);
            else
                return x.is_feasible(state, problem_, context);
        }, u);
        if (!ok) return false;
    }
    return true;
}

// ==================== EVALUATOR FREE FUNCTIONS ====================
// Work at the evaluator level (sequence-aware): take an Evaluatable evaluator.

namespace evaluator {

// Sums the soft-goal score across all sequences.
template<concepts::ConstraintEvaluator E>
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
template<concepts::ConstraintEvaluator E>
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

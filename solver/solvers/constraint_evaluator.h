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
concept PartialConsraintEvaluator = ConstraintEvaluator<E> && requires(
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

static_assert(ConstraintEvaluator<BaseEvaluator>,
    "ConstraintEvaluator must satisfy Evaluatable");

// ==================== POLICY EVALUATOR ====================

// Template evaluator that combines policies with the remaining problem constraints.
//
// P must satisfy policies::Evaluatable (penalty/evaluate/is_satisfied/is_feasible).
// If P additionally satisfies PartiallyEvaluatable, the partial_* methods dispatch
// to P's optimized per-(class_id, group) overloads; otherwise they fall back to
// full-state evaluation.
//
// The active sequence is set once per backtracking pass via set_sequence().
//
// Construction:
//   - Each policy replaces the constraint whose id matches policy.id.
//   - Unmatched constraints are kept in constraints_ and evaluated as normal.
//   - Default: empty policies vector — pure constraint evaluation, no policies active.
//
// Helpers follow the hard/soft split:
//   constraints_in / policies_in  — hard constraints/policies for this sequence
//   goals_in / policy_goals_in    — soft constraints/policies for this sequence
//   *_up_to / *_before            — all (hard+soft) for score and are_feasible
template<policies::Evaluatable P>
class PolicyEvaluator
{
public:
    explicit PolicyEvaluator(const TimeTableProblem& problem, std::vector<P> policies = {})
        : problem_(problem), policies_(std::move(policies)), sequence_(0)
    {
        std::unordered_set<int> claimed;
        for (const auto& p : policies_)
            claimed.insert(p.id);

        for (const auto& c : problem.get_constraints())
        {
            const int id = std::visit([](const auto& cv) { return cv.id; }, c);
            if (!claimed.contains(id))
                constraints_.push_back(c);
        }
    }

    void set_sequence(const int sequence) { sequence_ = sequence; }
    int  get_sequence() const             { return sequence_; }

    [[nodiscard]] SequenceContext score(const TimeTableState& state) const;
    void update_context(SequenceContext& context, const TimeTableState& state) const;
    [[nodiscard]] double evaluate(const TimeTableState& state) const;
    [[nodiscard]] bool   are_satisfied(const TimeTableState& state) const;
    [[nodiscard]] bool   are_feasible(const TimeTableState& state, const SequenceContext& context) const;

    // Partial evaluation — operates on the single (class_id, group) assignment that
    // was just placed in the state.  For policies that satisfy PartiallyEvaluatable
    // the dedicated partial overload is called; for everything else the full-state
    // method is used as a fallback.
    [[nodiscard]] double partial_evaluate(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_satisfied(const TimeTableState& state, int class_id, int group) const;
    [[nodiscard]] bool   partial_are_feasible(const TimeTableState& state, const SequenceContext& context, int class_id, int group) const;

private:
    const TimeTableProblem& problem_;
    std::vector<P> policies_;
    std::vector<solver_models::ConstraintVariant> constraints_;
    int sequence_;

    // Hard policies in this sequence — used by are_satisfied.
    [[nodiscard]] auto policies_in(const int seq) const
    {
        return policies_ | std::views::filter([seq](const P& p)
            { return p.sequence == seq && p.hard; });
    }
    // Soft policies in this sequence — used by evaluate.
    [[nodiscard]] auto policy_goals_in(const int seq) const
    {
        return policies_ | std::views::filter([seq](const P& p)
            { return p.sequence == seq && !p.hard; });
    }
    // All policies up to and including sequence — used by score.
    [[nodiscard]] auto policies_up_to(const int seq) const
    {
        return policies_ | std::views::filter([seq](const P& p)
            { return p.sequence <= seq; });
    }
    // All policies strictly before sequence — used by are_feasible.
    [[nodiscard]] auto policies_before(const int seq) const
    {
        return policies_ | std::views::filter([seq](const P& p)
            { return p.sequence < seq; });
    }

    // Hard constraints in this sequence — used by are_satisfied.
    [[nodiscard]] auto constraints_in(const int seq) const
    {
        return constraints_ | std::views::filter([seq](const solver_models::ConstraintVariant& c)
        {
            return std::visit([seq](const auto& cv)
                { return cv.sequence == seq && cv.hard; }, c);
        });
    }
    // Soft constraints in this sequence — used by evaluate.
    [[nodiscard]] auto goals_in(const int seq) const
    {
        return constraints_ | std::views::filter([seq](const solver_models::ConstraintVariant& c)
        {
            return std::visit([seq](const auto& cv)
                { return cv.sequence == seq && !cv.hard; }, c);
        });
    }
    // All constraints up to and including sequence — used by score.
    [[nodiscard]] auto constraints_up_to(const int seq) const
    {
        return constraints_ | std::views::filter([seq](const solver_models::ConstraintVariant& c)
        {
            return std::visit([seq](const auto& cv)
                { return cv.sequence <= seq; }, c);
        });
    }
    // All constraints strictly before sequence — used by are_feasible.
    [[nodiscard]] auto constraints_before(const int seq) const
    {
        return constraints_ | std::views::filter([seq](const solver_models::ConstraintVariant& c)
        {
            return std::visit([seq](const auto& cv)
                { return cv.sequence < seq; }, c);
        });
    }
};

// -------------------- Inline implementations --------------------

// Fills context with penalties for every policy and constraint up to sequence.
template<policies::Evaluatable P>
SequenceContext PolicyEvaluator<P>::score(
    const TimeTableState& state) const
{
    SequenceContext context(policies_.size() + constraints_.size());

    for (const auto& p : policies_up_to(sequence_))
        context[p.id] = p.penalty(state, problem_);

    for (const auto& c : constraints_up_to(sequence_))
        std::visit([&](const auto& cv)
            { context[cv.id] = cv.penalty(state, problem_); }, c);

    return context;
}

// Updates context with lower penalties found in this sequence (hard + soft).
template<policies::Evaluatable P>
void PolicyEvaluator<P>::update_context(
    SequenceContext& context, const TimeTableState& state) const
{
    auto update = [&](const int id, const double pen)
    {
        if (context[id] != 0.0 && pen < context[id])
            context[id] = pen;
    };

    for (const auto& p : policies_in(sequence_))
        update(p.id, p.penalty(state, problem_));
    for (const auto& p : policy_goals_in(sequence_))
        update(p.id, p.penalty(state, problem_));

    for (const auto& c : constraints_in(sequence_))
        std::visit([&](const auto& cv){ update(cv.id, cv.penalty(state, problem_)); }, c);
    for (const auto& c : goals_in(sequence_))
        std::visit([&](const auto& cv){ update(cv.id, cv.penalty(state, problem_)); }, c);
}

// Sums soft goals (policies + constraints) for this sequence.
template<policies::Evaluatable P>
double PolicyEvaluator<P>::evaluate(
    const TimeTableState& state) const
{
    double total = 0.0;
    for (const auto& p : policy_goals_in(sequence_))
        total += p.evaluate(state, problem_);
    for (const auto& c : goals_in(sequence_))
        std::visit([&](const auto& cv){ total += cv.evaluate(state, problem_); }, c);
    return total;
}

// True if all hard policies and hard constraints in this sequence are satisfied.
template<policies::Evaluatable P>
bool PolicyEvaluator<P>::are_satisfied(
    const TimeTableState& state) const
{
    for (const auto& p : policies_in(sequence_))
        if (!p.is_satisfied(state, problem_)) return false;
    for (const auto& c : constraints_in(sequence_))
    {
        const bool ok = std::visit([&](const auto& cv)
            { return cv.is_satisfied(state, problem_); }, c);
        if (!ok) return false;
    }
    return true;
}

// True if all policies and constraints from previous sequences are still feasible.
template<policies::Evaluatable P>
bool PolicyEvaluator<P>::are_feasible(
    const TimeTableState& state, const SequenceContext& context) const
{
    for (const auto& p : policies_before(sequence_))
        if (!p.is_feasible(state, problem_, context)) return false;
    for (const auto& c : constraints_before(sequence_))
    {
        const bool ok = std::visit([&](const auto& cv)
            { return cv.is_feasible(state, problem_, context); }, c);
        if (!ok) return false;
    }
    return true;
}

// Sums soft goals for this sequence, using per-(class_id,group) overloads when available.
template<policies::Evaluatable P>
double PolicyEvaluator<P>::partial_evaluate(
    const TimeTableState& state, const int class_id, const int group) const
{
    double total = 0.0;
    for (const auto& p : policy_goals_in(sequence_))
    {
        if constexpr (policies::PartiallyEvaluatable<P>)
            total += p.partial_evaluate(state, problem_, class_id, group);
        else
            total += p.evaluate(state, problem_);
    }
    for (const auto& c : goals_in(sequence_))
        std::visit([&](const auto& cv){ total += cv.evaluate(state, problem_); }, c);
    return total;
}

// True if all hard policies/constraints in this sequence are satisfied,
// using per-(class_id,group) overloads for policies when available.
template<policies::Evaluatable P>
bool PolicyEvaluator<P>::partial_are_satisfied(
    const TimeTableState& state, const int class_id, const int group) const
{
    for (const auto& p : policies_in(sequence_))
    {
        bool ok;
        if constexpr (policies::PartiallyEvaluatable<P>)
            ok = p.partial_is_satisfied(state, problem_, class_id, group);
        else
            ok = p.is_satisfied(state, problem_);
        if (!ok) return false;
    }
    for (const auto& c : constraints_in(sequence_))
    {
        const bool ok = std::visit([&](const auto& cv)
            { return cv.is_satisfied(state, problem_); }, c);
        if (!ok) return false;
    }
    return true;
}

// True if all policies/constraints from previous sequences are still feasible,
// using per-(class_id,group) overloads for policies when available.
template<policies::Evaluatable P>
bool PolicyEvaluator<P>::partial_are_feasible(
    const TimeTableState& state, const SequenceContext& context,
    const int class_id, const int group) const
{
    for (const auto& p : policies_before(sequence_))
    {
        bool ok;
        if constexpr (policies::PartiallyEvaluatable<P>)
            ok = p.partial_is_feasible(state, problem_, context, class_id, group);
        else
            ok = p.is_feasible(state, problem_, context);
        if (!ok) return false;
    }
    for (const auto& c : constraints_before(sequence_))
    {
        const bool ok = std::visit([&](const auto& cv)
            { return cv.is_feasible(state, problem_, context); }, c);
        if (!ok) return false;
    }
    return true;
}

// ==================== EVALUATOR FREE FUNCTIONS ====================
// Work at the evaluator level (sequence-aware): take an Evaluatable evaluator.

namespace evaluator {

// Sums the soft-goal score across all sequences.
template<ConstraintEvaluator E>
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
template<ConstraintEvaluator E>
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

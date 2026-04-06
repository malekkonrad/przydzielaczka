//
// Created by mateu on 05.04.2026.
//

#include "constraint_evaluator.h"

constraints::SequenceContext ConstraintEvaluator::score(const TimeTableState& state) const
{
    const auto& constraints = problem_.get_all_constraints(sequence_);
    const size_t n_constraints = problem_.get_constraints().size();

    constraints::SequenceContext context(n_constraints);
    for (const auto& constraint : constraints)
    {
        std::visit([&](const auto& c){ context.best_scores[c.id] = c.penalty(state, problem_); }, constraint);
    }
    return context;
}

void ConstraintEvaluator::update_context(constraints::SequenceContext& context, const TimeTableState& state) const
{
    const auto& constraints = problem_.get_constraints(sequence_);
    for (const auto& constraint : constraints)
    {
        std::visit([&](const auto& c)
        {
            if (context.best_scores[c.id] != 0.0)
            {
                const auto penalty = c.penalty(state, problem_);
                if (penalty < context.best_scores[c.id])
                {
                    context.best_scores[c.id] = penalty;
                }
            }
        }, constraint);
    }
}

double ConstraintEvaluator::evaluate(const TimeTableState& state) const
{
    const auto& goals = problem_.get_goals(sequence_);
    return constraints::evaluate_all(goals, problem_, state);
}

bool ConstraintEvaluator::are_satisfied(const TimeTableState& state) const
{
    const auto& hard_constraints = problem_.get_hard_constraints(sequence_);
    return constraints::are_satisfied(hard_constraints, problem_, state);
}

bool ConstraintEvaluator::are_feasible(
    const TimeTableState& state,
    const constraints::SequenceContext& context) const
{
    const auto& previous_constraints = problem_.get_previous_constraints(sequence_);
    return constraints::are_feasible(previous_constraints, problem_, state, context);
}

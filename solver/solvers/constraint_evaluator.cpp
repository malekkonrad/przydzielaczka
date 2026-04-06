//
// Created by mateu on 05.04.2026.
//

#include "constraint_evaluator.h"
#include "sequence_context.h"

SequenceContext BaseEvaluator::score(const TimeTableState& state) const
{
    const auto& constraints = problem_.constraints_up_to(sequence_);
    const size_t n_constraints = problem_.get_constraints().size();

    SequenceContext context(n_constraints);
    for (const auto& constraint : constraints)
    {
        std::visit([&](const auto& c){ context[c.id] = c.penalty(state, problem_); }, constraint);
    }
    return context;
}

void BaseEvaluator::update_context(SequenceContext& context, const TimeTableState& state) const
{
    const auto& constraints = problem_.constraints_in(sequence_);
    for (const auto& constraint : constraints)
    {
        std::visit([&](const auto& c)
        {
            if (context[c.id] != 0.0)
            {
                const auto penalty = c.penalty(state, problem_);
                if (penalty < context[c.id])
                {
                    context[c.id] = penalty;
                }
            }
        }, constraint);
    }
}

double BaseEvaluator::evaluate(const TimeTableState& state) const
{
    const auto& goals = problem_.goals_in(sequence_);
    return constraints::evaluate_all(goals, problem_, state);
}

bool BaseEvaluator::are_satisfied(const TimeTableState& state) const
{
    const auto& hard_constraints = problem_.hard_constraints_in(sequence_);
    return constraints::are_satisfied(hard_constraints, problem_, state);
}

bool BaseEvaluator::are_feasible(
    const TimeTableState& state,
    const SequenceContext& context) const
{
    const auto& previous_constraints = problem_.constraints_before(sequence_);
    return constraints::are_feasible(previous_constraints, problem_, state, context);
}

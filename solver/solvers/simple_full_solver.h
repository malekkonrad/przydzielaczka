//
// Created by mateu on 04.04.2026.
//

#pragma once

#include <constraints.h>
#include <solver_base.h>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <constraint_evaluator.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <utility>
#include <variant>
#include <vector>

// SimpleFullSolver — a concrete, non-template backtracking solver.
//
// Algorithm (one pass per sequence):
//   1. Backtrack over all classes, pruning on time overlap.
//   2. At the leaf, reject states that violate any hard constraint for this
//      sequence or fail the slack-feasibility check from the previous sequence.
//   3. Score accepted states by the soft goals of the current sequence.
//   4. Build a SequenceContext from the minimum per-constraint penalty across
//      all optimal-score states (most permissive bound for the next sequence).
//   5. Repeat for the next sequence.
//
// This is the baseline: correctness over performance.
// Constraint-level pruning (early backtracking) is left for derived solvers.
class SimpleFullSolver : public SolverBase<NoopEvaluator>
{
public:
    explicit SimpleFullSolver(const TimeTableProblem& problem, const solver::config& config)
        : SolverBase<NoopEvaluator>(problem, config) {}

    std::vector<TimeTableState> solve() override;

private:

    // ---------- time overlap ----------

    static bool overlaps(const solver_models::Class& a, const solver_models::Class& b)
    {
        if (a.day != b.day) return false;
        if (!(a.week & b.week).any()) return false;
        return a.start_time < b.end_time && b.start_time < a.end_time;
    }

    // True if the candidate class (class_id, group) time-conflicts with any
    // class already assigned in state (excluding itself).
    static bool has_time_conflict(int class_id, int group,
                                  const TimeTableState& state,
                                  const TimeTableProblem& problem)
    {
        const auto& candidate = problem.get_group(class_id, group);
        for (const int other_id : state.get_assigned_classes())
        {
            if (other_id == class_id) continue;
            const int other_group = state.get_groups()[other_id];
            if (overlaps(candidate, problem.get_group(other_id, other_group)))
                return true;
        }
        return false;
    }

    // ---------- constraint checks (leaf) ----------

    // True if any hard constraint with sequence <= seq is violated.
    static bool violates_hard(const TimeTableState& state,
                               const TimeTableProblem& problem,
                               int seq)
    {
        for (const auto& cv : problem.get_constraints())
        {
            const bool violated = std::visit([&](const auto& c) -> bool {
                return c.hard && c.sequence <= seq && c.penalty(state, problem) > 0.0;
            }, cv);
            if (violated) return true;
        }
        return false;
    }

    // False if any constraint from a previous sequence fails its slack check.
    static bool satisfies_context(const TimeTableState& state,
                                  const TimeTableProblem& problem,
                                  const constraints::SequenceContext& context,
                                  int seq)
    {
        for (const auto& cv : problem.get_constraints())
        {
            const bool infeasible = std::visit([&](const auto& c) -> bool {
                return c.sequence < seq && !c.is_feasible(state, problem, context);
            }, cv);
            if (infeasible) return false;
        }
        return true;
    }

    // ---------- scoring ----------

    // Sum of evaluate() (weight * penalty) for soft constraints of exactly seq.
    static double score_sequence(const TimeTableState& state,
                                 const TimeTableProblem& problem,
                                 int seq)
    {
        double total = 0.0;
        for (const auto& cv : problem.get_constraints())
            std::visit([&](const auto& c) {
                if (!c.hard && c.sequence == seq)
                    total += c.evaluate(state, problem);
            }, cv);
        return total;
    }

    // ---------- context building ----------

    // Build a SequenceContext by taking the minimum per-constraint penalty
    // across all provided solutions (most permissive bound for the next sequence).
    static constraints::SequenceContext make_context(
        const std::vector<TimeTableState>& pool,
        const std::vector<std::size_t>& indices,
        const TimeTableProblem& problem)
    {
        const auto& all = problem.get_constraints();
        constraints::SequenceContext ctx;
        ctx.best_scores.assign(all.size(), std::numeric_limits<double>::infinity());

        for (const std::size_t idx : indices)
        {
            for (const auto& cv : all)
                std::visit([&](const auto& c) {
                    ctx.best_scores[c.id] = std::min(ctx.best_scores[c.id],
                                                     c.penalty(pool[idx], problem));
                }, cv);
        }
        return ctx;
    }
};

// ---------------------------------------------------------------------------
// solve() — defined inline here because SimpleFullSolver is header-only.
// ---------------------------------------------------------------------------

inline std::vector<TimeTableState> SimpleFullSolver::solve()
{
    const int n_classes = static_cast<int>(problem_.class_size());
    const int n_seqs    = static_cast<int>(problem_.sequence_size());
    const bool verbose  = config_.verbose;

    // If no sequences are defined, do a single pass with no constraint checks.
    const int n_passes = std::max(1, n_seqs);

    constraints::SequenceContext context; // empty — no prior sequence
    std::vector<TimeTableState> solutions;

    for (int seq = 0; seq < n_passes; ++seq)
    {
        if (verbose)
        {
            std::cout << "\n=== Sequence " << seq << " ===" << std::endl;
        }

        const auto& previous_constraints = problem_.get_previous_constraints(seq);
        const auto& hard_constraints = problem_.get_hard_constraints(seq);
        std::vector<TimeTableState> found;
        TimeTableState current(n_classes);

        std::function<void(int)> backtrack = [&](const int depth)
        {
            if (depth == n_classes)
            {
                found.push_back(current);
                return;
            }

            const int class_id = depth;
            const int max_group = problem_.get_max_group(class_id);

            for (int group = 0; group <= max_group; ++group)
            {
                current.assign(class_id, group);
                const bool are_feasible = constraints::are_feasible(previous_constraints, problem_, current, context);
                if (are_feasible)
                {
                    const bool are_satisfied = constraints::are_satisfied(hard_constraints, problem_, current);
                    if (are_satisfied)
                    {
                        backtrack(depth + 1);
                    }
                }
                current.unassign(class_id);
            }
        };

        backtrack(0);

        if (verbose)
        {
            std::cout << "  found " << found.size() << " candidate(s)" << std::endl;
        }

        if (found.empty())
        {
            if (verbose)
            {
                std::cout << "  no solutions in sequence " << seq << " — stopping" << std::endl;
            }
            break;
        }

        if (seq < n_seqs)
        {
            // Score each candidate by the soft goals of this sequence.
            std::vector<std::pair<double, std::size_t>> scored;
            scored.reserve(found.size());
            for (std::size_t i = 0; i < found.size(); ++i)
            {
                scored.emplace_back(score_sequence(found[i], problem_, seq), i);
            }

            std::sort(scored.begin(), scored.end());

            const double best_score = scored[0].first;
            if (verbose)
                std::cout << "  best score: " << best_score << std::endl;

            // Collect all indices that share the best score.
            std::vector<std::size_t> optimal_indices;
            for (const auto& [s, idx] : scored)
            {
                if (s > best_score) break;
                optimal_indices.push_back(idx);
            }

            // Build context: minimum penalty per constraint across optimal solutions.
            // This gives the most permissive slack bound for the next sequence.
            context = make_context(found, optimal_indices, problem_);

            // Expose up to max_solutions optimal states.
            solutions.clear();
            for (const std::size_t idx : optimal_indices)
            {
                if (solutions.size() >= config_.max_solutions) break;
                solutions.push_back(std::move(found[idx]));
            }
        }
        else
        {
            // No sequences — return up to max_solutions conflict-free states.
            solutions.clear();
            const std::size_t n_out = std::min(found.size(), config_.max_solutions);
            for (std::size_t i = 0; i < n_out; ++i)
                solutions.push_back(std::move(found[i]));
        }

        if (verbose)
            std::cout << "  returning " << solutions.size() << " solution(s)" << std::endl;
    }

    return solutions;
}
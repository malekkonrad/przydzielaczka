//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <chrono>
#include <time_table_problem.h>
#include <time_table_state.h>
#include <constraint_evaluator.h>
#include <solution_set.h>
#include <stats.h>

#include "solver_config.h"
#include "traits.h"

// SolverBase<Traits> is the base for concrete solvers.
// It owns the problem reference, constructs the evaluator from the Traits type,
// and exposes both to subclasses.
//
// Traits must be a BasicSolverTraits specialisation.  The evaluator type is
// derived automatically as BasicConstraintEvaluator<Traits>.
//
// Stats helpers (protected):
//   Call stats_begin_sequence()  at the start of each sequence.
//   Call stats_record_*()        inside the search to count events.
//   Call stats_commit_sequence() after the search ends; it timestamps and
//                                rolls the SequenceStats into SolverStats.
//   Call stats_set_solutions_kept() once after trimming the solution set.

template<SolverTraitsConcept Traits = SolverTraits>
class SolverBase
{
public:
    // TODO inform if the evaluator supports partial and change Traits
    using Evaluator = BasicConstraintEvaluator<Traits>;

    explicit SolverBase(const TimeTableProblem& problem, const solver::config& config)
        : problem_(problem), config_(config), evaluator_(problem) {}

    virtual ~SolverBase() = default;

    SolverBase(const SolverBase&)            = delete;
    SolverBase& operator=(const SolverBase&) = delete;

    virtual BoundedSolutionSet<SequenceContext> solve() = 0;

    [[nodiscard]] const SolverStats& stats() const { return stats_; }

protected:
    // -----------------------------------------------------------------------
    // Stats helpers — thin wrappers so subclasses never touch stats_ directly.
    // -----------------------------------------------------------------------

    [[nodiscard]] long long count_leaves(const TimeTableState& state) const
    {
        long long leaves = 1;
        const auto& groups = state.get_raw_groups();
        for (int i = 0; i < groups.size(); ++i)
        {
            const auto group = groups[i];
            if (group == TimeTableState::UNASSIGNED)
            {
                leaves *= problem_.get_max_group(i) * 2LL;
            }
        }
        return leaves;
    }

    [[nodiscard]] long long count_leaves() const
    {
        long long leaves = 1;;
        for (int i = 0; i < problem_.class_size(); ++i)
        {
            leaves *= problem_.get_max_group(i) * 2LL;

        }
        return leaves;
    }

    // Call once at the beginning of a sequence.
    // nodes_total  — total tree nodes (internal + leaves) for progress reporting (0 = unknown).
    // leaves_total — total leaf nodes; denominator for per-mechanism leaf percentages (0 = unknown).
    void stats_begin_sequence(const int seq_index,
                              const long long leaves_total = 0)
    {
        seq_stats_                = SequenceStats{};
        seq_stats_.sequence_index = seq_index;
        seq_stats_.leaves_total   = leaves_total;
        seq_start_  = std::chrono::steady_clock::now();
        last_print_ = seq_start_;
        SequenceStats::print_header();
    }

    // Call once after the search for a sequence finishes.
    // Stamps elapsed time and rolls the SequenceStats into SolverStats.
    void stats_commit_sequence()
    {
        seq_stats_.time_ms =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - seq_start_).count();
        stats_.accumulate(seq_stats_);
    }

    // Individual event counters — call at the relevant point in the search.
    // The `leaves` parameter is the number of leaf nodes in the eliminated subtree.
    void stats_record_visited()                                   { ++seq_stats_.nodes_visited; }
    void stats_record_pruned         (const long long leaves = 0) { ++seq_stats_.nodes_pruned;          seq_stats_.leaves_pruned          += leaves; }
    void stats_record_feasibility_cut(const long long leaves = 0) { ++seq_stats_.nodes_feasibility_cut; seq_stats_.leaves_feasibility_cut += leaves; }
    void stats_record_constraint_cut (const long long leaves = 0) { ++seq_stats_.nodes_constraint_cut;  seq_stats_.leaves_constraint_cut  += leaves; }

    // Call when a complete, evaluated solution is found.
    void stats_record_solution(const double eval)
    {
        ++seq_stats_.leaves_total;
        ++seq_stats_.solutions_found;
        if (eval < seq_stats_.best_eval)
        {
            seq_stats_.best_eval = eval;
        }
    }

    // Call after trimming the solution set to record how many were kept overall.
    void stats_set_solutions_kept(const int n) { stats_.solutions_kept = n; }

    // Read-only view of the in-progress sequence stats.
    [[nodiscard]] const SequenceStats& current_seq_stats() const { return seq_stats_; }

    // Inplace print throttled to at most once per `interval_ms` milliseconds.
    // Safe to call on every node — does nothing if called too soon.
    void stats_print_inplace_throttled(const double interval_ms = 200.0)
    {
        const auto   now     = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double, std::milli>(now - seq_start_).count();
        const double since   = std::chrono::duration<double, std::milli>(now - last_print_).count();
        if (since < interval_ms) return;
        seq_stats_.print_inplace(elapsed);
        last_print_ = now;
    }

    // Inplace print with live elapsed time, always — use sparingly.
    void stats_print_inplace() const
    {
        const double elapsed =
            std::chrono::duration<double, std::milli>(
                std::chrono::steady_clock::now() - seq_start_).count();
        seq_stats_.print_inplace(elapsed);
    }

    // -----------------------------------------------------------------------

    const TimeTableProblem& problem_;
    const solver::config&   config_;
    Evaluator               evaluator_;
    SolverStats             stats_;

private:
    SequenceStats                         seq_stats_;
    std::chrono::steady_clock::time_point seq_start_;
    std::chrono::steady_clock::time_point last_print_;
};
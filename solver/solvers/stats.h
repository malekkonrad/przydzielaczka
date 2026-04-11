//
// Created by mateu on 10.04.2026.
//

#pragma once

#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace ansi {
    inline constexpr auto reset   = "\033[0m";
    inline constexpr auto bold    = "\033[1m";
    inline constexpr auto dim     = "\033[2m";
    inline constexpr auto cyan    = "\033[36m";
    inline constexpr auto green   = "\033[32m";
    inline constexpr auto yellow  = "\033[33m";
    inline constexpr auto red     = "\033[31m";
    inline constexpr auto magenta = "\033[35m";
    inline constexpr auto white   = "\033[37m";
} // namespace ansi

namespace display
{
    inline constexpr int SEQUENCE_WIDTH              = 3;
    inline constexpr int PROGRESS_WIDTH              = 11;  // leaf counts (processed, total)
    inline constexpr int VISITED_WIDTH               = 8;   // nodes visited
    inline constexpr int FOUND_WIDTH                 = 6;   // solutions found
    inline constexpr int NODES_PRUNED_CUT_WIDTH      = 8;   // node counts per cut type
    inline constexpr int NODES_FEASIBLE_CUT_WIDTH    = 8;
    inline constexpr int NODES_CONSTRAINT_CUT_WIDTH  = 8;
    inline constexpr int BEST_WIDTH                  = 7;   // best eval (2 d.p.)
    inline constexpr int TIME_WIDTH                  = 7;   // elapsed ms (1 d.p.)
    inline constexpr int PERCENT_WIDTH               = 5;
    inline constexpr int ADDITIONAL_PERCENT_WIDTH    = 9;
} // namespace display

struct SequenceStats
{
    int       sequence_index         = 0;
    int       solutions_found        = 0;

    // Node counts (every backtrack() call = one node).
    long long nodes_visited          = 0;
    long long nodes_pruned           = 0;   // cut by B&B lower-bound check
    long long nodes_feasibility_cut  = 0;   // cut by partial_are_feasible()
    long long nodes_constraint_cut   = 0;   // cut by partial_are_satisfied()

    // Leaf counts — each cut eliminates a whole subtree; these accumulate
    // the leaf-node count of every eliminated subtree.
    long long leaves_total           = 0;   // total leaves in the full search tree
    long long leaves_pruned          = 0;   // leaves eliminated by B&B pruning
    long long leaves_feasibility_cut = 0;   // leaves eliminated by feasibility cuts
    long long leaves_constraint_cut  = 0;   // leaves eliminated by constraint cuts

    double    best_eval              = std::numeric_limits<double>::infinity();
    double    time_ms                = 0.0;

    // Prints the header of the table
    static void print_header();

    // Overwrites the current line — call repeatedly inside a sequence loop.
    // elapsed_ms: live elapsed time from the caller; falls back to time_ms if negative.
    void print_inplace(double elapsed_ms = -1.0) const;

    // Overwrites the inplace line with the final committed stats, then newlines.
    // Call once after stats_commit_sequence() so time_ms is stamped.
    void print_final() const;

private:
    void print_line(double display_time) const;
};

struct SolverStats
{
    // --- Solutions ---
    int    solutions_found    = 0;
    int    solutions_kept     = 0;

    // --- Search tree (nodes) ---
    long long nodes_visited          = 0;
    long long nodes_pruned           = 0;
    long long nodes_feasibility_cut  = 0;
    long long nodes_constraint_cut   = 0;

    // --- Search tree (leaves eliminated by each mechanism) ---
    long long leaves_pruned          = 0;
    long long leaves_feasibility_cut = 0;
    long long leaves_constraint_cut  = 0;

    // --- Timing ---
    double total_time_ms    = 0.0;

    // --- Per-sequence breakdown ---
    std::vector<SequenceStats> per_sequence;

    void accumulate(const SequenceStats& s);

    void print() const;
};
//
// Created by mateu on 10.04.2026.
//

#pragma once

#include <iomanip>
#include <iostream>
#include <limits>
#include <vector>

namespace ansi {
    inline constexpr const char* reset   = "\033[0m";
    inline constexpr const char* bold    = "\033[1m";
    inline constexpr const char* dim     = "\033[2m";
    inline constexpr const char* cyan    = "\033[36m";
    inline constexpr const char* green   = "\033[32m";
    inline constexpr const char* yellow  = "\033[33m";
    inline constexpr const char* red     = "\033[31m";
    inline constexpr const char* magenta = "\033[35m";
    inline constexpr const char* white   = "\033[37m";
} // namespace ansi

struct SequenceStats
{
    int       sequence_index         = 0;
    int       solutions_found        = 0;

    // Node counts (every backtrack() call = one node).
    long long nodes_visited          = 0;
    long long nodes_pruned           = 0;   // cut by B&B lower-bound check
    long long nodes_feasibility_cut  = 0;   // cut by partial_are_feasible()
    long long nodes_constraint_cut   = 0;   // cut by partial_are_satisfied()
    long long nodes_total            = 0;   // total tree nodes (internal + leaves)

    // Leaf counts — each cut eliminates a whole subtree; these accumulate
    // the leaf-node count of every eliminated subtree.
    long long leaves_total           = 0;   // total leaves in the full search tree
    long long leaves_pruned          = 0;   // leaves eliminated by B&B pruning
    long long leaves_feasibility_cut = 0;   // leaves eliminated by feasibility cuts
    long long leaves_constraint_cut  = 0;   // leaves eliminated by constraint cuts

    double    best_eval              = std::numeric_limits<double>::infinity();
    double    time_ms                = 0.0;

    // Overwrites the current line — call repeatedly inside a sequence loop.
    // elapsed_ms: live elapsed time from the caller; falls back to time_ms if negative.
    void print_inplace(const double elapsed_ms = -1.0) const
    {
        print_line("\r", elapsed_ms);
        std::cout << std::flush;
    }

    // Overwrites the inplace line with the final committed stats, then newlines.
    // Call once after stats_commit_sequence() so time_ms is stamped.
    void print_final() const
    {
        print_line("\r", time_ms);
        std::cout << "\n";
    }

private:
    void print_line(const char* prefix, const double display_time) const
    {
        const long long processed_leaves = leaves_pruned + leaves_feasibility_cut + leaves_constraint_cut;
        const double node_pct = leaves_total > 0
            ? 100.0 * static_cast<double>(processed_leaves)       / static_cast<double>(leaves_total) : 0.0;
        const double lb_pct   = leaves_total > 0
            ? 100.0 * static_cast<double>(leaves_pruned)          / static_cast<double>(leaves_total) : 0.0;
        const double fe_pct   = leaves_total > 0
            ? 100.0 * static_cast<double>(leaves_feasibility_cut) / static_cast<double>(leaves_total) : 0.0;
        const double co_pct   = leaves_total > 0
            ? 100.0 * static_cast<double>(leaves_constraint_cut)  / static_cast<double>(leaves_total) : 0.0;

        std::cout << prefix
                  << ansi::bold << ansi::cyan  << "[seq " << std::setw(3) << sequence_index << "]" << ansi::reset
                  << ansi::dim  << "  progress: " << ansi::reset
                  << ansi::white << std::setw(10) << processed_leaves << "/" << std::setw(10) << leaves_total << ansi::reset
                  << ansi::yellow << " (" << std::setw(5) << std::fixed << std::setprecision(1) << node_pct << "%)" << ansi::reset
                  << ansi::dim  << "  visited: " << ansi::reset
                  << ansi::white << std::setw(8) << nodes_visited << ansi::reset
                  << ansi::dim  << "  found: " << ansi::reset
                  << ansi::green << std::setw(6) << solutions_found << ansi::reset
                  << ansi::dim  << "  lb=" << ansi::reset
                  << ansi::red    << std::setw(6) << nodes_pruned           << " (" << std::setw(5) << std::fixed << std::setprecision(1) << lb_pct << "%)" << ansi::reset
                  << ansi::dim  << "  fe=" << ansi::reset
                  << ansi::yellow << std::setw(6) << nodes_feasibility_cut  << " (" << std::setw(5) << fe_pct << "%)" << ansi::reset
                  << ansi::dim  << "  co=" << ansi::reset
                  << ansi::magenta<< std::setw(6) << nodes_constraint_cut   << " (" << std::setw(5) << co_pct << "%)" << ansi::reset
                  << ansi::dim  << "  best: " << ansi::reset
                  << ansi::cyan  << std::setw(8) << std::fixed << std::setprecision(2) << best_eval << ansi::reset
                  << ansi::dim  << "  time: " << ansi::reset
                  << ansi::white << std::setw(8) << std::fixed << std::setprecision(1) << display_time << " ms" << ansi::reset;
    }

public:
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

    void accumulate(const SequenceStats& s)
    {
        solutions_found        += s.solutions_found;
        nodes_visited          += s.nodes_visited;
        nodes_pruned           += s.nodes_pruned;
        nodes_feasibility_cut  += s.nodes_feasibility_cut;
        nodes_constraint_cut   += s.nodes_constraint_cut;
        leaves_pruned          += s.leaves_pruned;
        leaves_feasibility_cut += s.leaves_feasibility_cut;
        leaves_constraint_cut  += s.leaves_constraint_cut;
        total_time_ms          += s.time_ms;
        per_sequence.push_back(s);
    }

    void print() const
    {
        std::cout << "\n=== SolverStats ===\n"
                  << "  solutions found:       " << solutions_found        << "\n"
                  << "  solutions kept:        " << solutions_kept         << "\n"
                  << "  nodes visited:         " << nodes_visited          << "\n"
                  << "  nodes pruned (B&B):    " << nodes_pruned           << "\n"
                  << "  nodes feasibility cut: " << nodes_feasibility_cut  << "\n"
                  << "  nodes constraint cut:  " << nodes_constraint_cut   << "\n"
                  << "  leaves pruned (B&B):   " << leaves_pruned          << "\n"
                  << "  leaves feasibility:    " << leaves_feasibility_cut << "\n"
                  << "  leaves constraint:     " << leaves_constraint_cut  << "\n"
                  << "  total time:            " << std::fixed << std::setprecision(2) << total_time_ms << " ms\n";

        if (!per_sequence.empty())
        {
            std::cout << "\n  --- per sequence ---\n";
            for (const auto& s : per_sequence)
            {
                const double lb_pct = s.leaves_total > 0
                    ? 100.0 * static_cast<double>(s.leaves_pruned)          / static_cast<double>(s.leaves_total) : 0.0;
                const double fe_pct = s.leaves_total > 0
                    ? 100.0 * static_cast<double>(s.leaves_feasibility_cut) / static_cast<double>(s.leaves_total) : 0.0;
                const double co_pct = s.leaves_total > 0
                    ? 100.0 * static_cast<double>(s.leaves_constraint_cut)  / static_cast<double>(s.leaves_total) : 0.0;

                std::cout << "  [seq " << std::setw(3) << s.sequence_index << "]"
                          << "  found: "   << std::setw(6)  << s.solutions_found
                          << "  visited: " << std::setw(8)  << s.nodes_visited
                          << "  leaves: lb=" << std::setw(5) << std::fixed << std::setprecision(1) << lb_pct << "%"
                          <<           " fe=" << std::setw(5) << fe_pct << "%"
                          <<           " co=" << std::setw(5) << co_pct << "%"
                          << "  best: "    << std::setw(10) << std::fixed << std::setprecision(4) << s.best_eval
                          << "  time: "    << std::setw(8)  << std::fixed << std::setprecision(2) << s.time_ms << " ms\n";
            }
        }
        std::cout << std::endl;
    }
};
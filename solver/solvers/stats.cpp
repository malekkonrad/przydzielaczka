//
// Created by mateu on 11.04.2026.
//

#include "stats.h"


void SequenceStats::print_header()
{
    constexpr int pr_w   = display::PROGRESS_WIDTH + display::ADDITIONAL_PERCENT_WIDTH;
    constexpr int lb_w   = display::NODES_PRUNED_CUT_WIDTH + display::ADDITIONAL_PERCENT_WIDTH;
    constexpr int fe_w   = display::NODES_FEASIBLE_CUT_WIDTH  +  display::ADDITIONAL_PERCENT_WIDTH;
    constexpr int co_w   = display::NODES_CONSTRAINT_CUT_WIDTH +display::ADDITIONAL_PERCENT_WIDTH;
    constexpr int time_w = display::TIME_WIDTH + 3;

    std::cout
        << std::left  << ansi::cyan    << std::setw(display::SEQUENCE_WIDTH)    << "seq" << ansi::reset
        << " | "
        << std::left  << ansi::white   << std::setw(pr_w)                       << "processed" << ansi::reset
        << " | "
        << std::right << ansi::white   << std::setw(display::VISITED_WIDTH)     << "visited" << ansi::reset
        << " | "
        << std::right << ansi::green   << std::setw(display::FOUND_WIDTH)       << "found" << ansi::reset
        << " | "
        << std::left  << ansi::red     << std::setw(lb_w)                       << "pruned nodes" << ansi::reset
        << " | "
        << std::left  << ansi::yellow  << std::setw(fe_w)                       << "fe nodes" << ansi::reset
        << " | "
        << std::left  << ansi::magenta << std::setw(co_w)                       << "co nodes" << ansi::reset
        << " | "
        << std::left  << ansi::red     << std::setw(display::PERCENT_WIDTH + 1) << "pr lf" << ansi::reset
        << " | "
        << std::left  << ansi::yellow  << std::setw(display::PERCENT_WIDTH + 1) << "fe lf" << ansi::reset
        << " | "
        << std::left  << ansi::magenta << std::setw(display::PERCENT_WIDTH + 1) << "co lf" << ansi::reset
        << " | "
        << std::right << ansi::cyan    << std::setw(display::BEST_WIDTH)        << "best" << ansi::reset
        << " | "
        << std::right << ansi::white   << std::setw(time_w)                     << "time(ms)" << ansi::reset
        << "\n";
}

void SequenceStats::print_inplace(const double elapsed_ms) const
{
    print_line(elapsed_ms);
    std::cout << std::flush;
}

void SequenceStats::print_final() const
{
    print_line(time_ms);
    std::cout << "\n";
}

void SequenceStats::print_line(const double display_time) const
{
    const long long processed_leaves = leaves_pruned + leaves_feasibility_cut + leaves_constraint_cut;
    const double node_pct = leaves_total > 0
        ? 100.0 * static_cast<double>(processed_leaves)       / static_cast<double>(leaves_total) : 0.0;
    const double lb_nodes_pct = nodes_visited > 0
        ? 100.0 * static_cast<double>(nodes_pruned)          / static_cast<double>(nodes_visited) : 0.0;
    const double fe_nodes_pct = nodes_visited > 0
        ? 100.0 * static_cast<double>(nodes_feasibility_cut) / static_cast<double>(nodes_visited) : 0.0;
    const double co_nodes_pct = nodes_visited > 0
        ? 100.0 * static_cast<double>(nodes_constraint_cut)  / static_cast<double>(nodes_visited) : 0.0;
    const double lb_pct   = leaves_total > 0
        ? 100.0 * static_cast<double>(leaves_pruned)          / static_cast<double>(leaves_total) : 0.0;
    const double fe_pct   = leaves_total > 0
        ? 100.0 * static_cast<double>(leaves_feasibility_cut) / static_cast<double>(leaves_total) : 0.0;
    const double co_pct   = leaves_total > 0
        ? 100.0 * static_cast<double>(leaves_constraint_cut)  / static_cast<double>(leaves_total) : 0.0;

    std::cout << std::right << std::fixed
        << "\r"
        << ansi::bold << ansi::cyan <<std::setw(display::SEQUENCE_WIDTH) << sequence_index
        << ansi::reset
        << " | "
        << ansi::white << std::setw(display::PROGRESS_WIDTH) << processed_leaves
        << ansi::reset
        << ansi::yellow << " (" << std::setw(5) << std::setprecision(1) << node_pct << "%)"
        << ansi::reset
        << " | "
        << ansi::white   << std::setw(display::VISITED_WIDTH) << nodes_visited << ansi::reset
        << " | "
        << ansi::green   << std::setw(display::FOUND_WIDTH) << solutions_found << ansi::reset
        << " | "
        << ansi::red
        << std::setw(display::NODES_PRUNED_CUT_WIDTH) << nodes_pruned
        << " (" << std::setw(display::PERCENT_WIDTH) << std::setprecision(1) << lb_nodes_pct << "%)"
        << ansi::reset
        << " | "
        << ansi::yellow
        << std::setw(display::NODES_FEASIBLE_CUT_WIDTH) << nodes_feasibility_cut
        << " (" << std::setw(display::PERCENT_WIDTH) << fe_nodes_pct << "%)"
        << ansi::reset
        << " | "
        << ansi::magenta
        << std::setw(display::NODES_CONSTRAINT_CUT_WIDTH) << nodes_constraint_cut
        << " (" << std::setw(display::PERCENT_WIDTH) << co_nodes_pct << "%)"
        << ansi::reset
        << " | "
        << ansi::red
        << std::setw(display::PERCENT_WIDTH) << std::setprecision(1) << lb_pct << "%"
        << ansi::reset
        << " | "
        << ansi::yellow
        << std::setw(display::PERCENT_WIDTH) << fe_pct << "%"
        << ansi::reset
        << " | "
        << ansi::magenta
        << std::setw(display::PERCENT_WIDTH) << co_pct << "%"
        << ansi::reset
        << " | "
        << ansi::cyan    << std::setw(display::BEST_WIDTH) << std::setprecision(1) << best_eval << ansi::reset
        << " | "
        << ansi::white   << std::setw(display::TIME_WIDTH) << std::setprecision(1) << display_time << " ms" << ansi::reset;
}


void SolverStats::accumulate(const SequenceStats& s)
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

void SolverStats::print() const
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
        SequenceStats::print_header();
        for (const auto& s : per_sequence)
        {
            s.print_final();
        }
    }
    std::cout << std::endl;
}
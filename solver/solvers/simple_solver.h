//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <solver_base.h>

#include <algorithm>
#include <functional>
#include <iostream>
#include <utility>

template<typename Evaluator>
class SimpleSolver : public SolverBase<Evaluator>
{
public:
    explicit SimpleSolver(const TimeTableProblem& problem, const solver::config& config)
        : SolverBase<Evaluator>(problem, config) {}

    std::vector<TimeTableState> solve() override;
};

// Implementation lives here because SimpleSolver is a template.
// Explicit instantiations for known evaluators are in simple_solver.cpp.
template<typename Evaluator>
std::vector<TimeTableState> SimpleSolver<Evaluator>::solve()
{
    const bool verbose = this->config_.verbose;
    // Only print the tree for the top kPrintDepth levels; below that use a
    // single overwritten progress line so deep searches stay fast.
    constexpr int kPrintDepth = 4;
    long long attempt = 0;

    // Enumerate every combination (Cartesian product of groups per subject),
    // pruning with has_conflict and scoring complete states.
    using ScoredState = std::pair<double, TimeTableState>;
    std::vector<ScoredState> all_solutions;

    const size_t n = this->problem_.class_size();
    const int int_n = static_cast<int>(n);
    TimeTableState current{n};
    bool stop = false;

    std::function<void(int)> backtrack = [&](const int depth)
    {
        if (depth == int_n)
        {
            all_solutions.emplace_back(this->evaluator_.score(current), current);

            // if (all_solutions.size() >= this->config_.max_keep_solutions)
            // {
            //     stop = true;
            // }
            // if (verbose)
            //     std::cout << "\n[solution #" << all_solutions.size()
            //               << " score=" << all_solutions.back().first << "]\n";
            return;
        }

        const int class_id = depth;
        const int max_g = this->problem_.get_max_group(class_id);

        for (int group = 0; group <= max_g; group++)
        {
            if (stop) break;
            current.assign(class_id, group);
            const bool conflict = this->evaluator_.has_conflict(class_id, group, current);

            if (verbose)
            {
                ++attempt;
                if (depth < kPrintDepth)
                {
                    // Box-drawing tree line, printed once per node.
                    for (int d = 0; d < depth; ++d) std::cout << "|  ";
                    std::cout << (group < max_g ? "+- " : "\\- ")
                              << class_id << ":" << group
                              << (conflict ? " x" : "") << "\n";
                }
                else
                {
                    // Deep nodes: overwrite a single progress line every 10k attempts.
                    if (attempt % 10000 == 0)
                        std::cout << "  [attempts=" << attempt
                                  << " solutions=" << all_solutions.size()
                                  << " depth=" << depth
                                  << " class=" << class_id << ":" << group << "]   \r"
                                  << std::flush;
                }
            }

            if (!conflict)
            {
                backtrack(depth + 1);
            }
            current.unassign(class_id);
        }
    };

    backtrack(0);

    // Sort ascending — lower score is better.
    std::sort(all_solutions.begin(), all_solutions.end(),
              [](const ScoredState& a, const ScoredState& b) {
                  return a.first < b.first;
              });

    const int n_out = static_cast<int>(std::min(this->config_.max_solutions, all_solutions.size()));
    std::vector<TimeTableState> result;
    result.reserve(n_out);
    for (int i = 0; i < n_out; i++)
    {
        std::cout << all_solutions[i].second << std::endl;
        result.push_back(std::move(all_solutions[i].second));
    }

    return result;
}

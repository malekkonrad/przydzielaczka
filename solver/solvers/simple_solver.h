//
// Created by mateu on 01.04.2026.
//

#pragma once

#include "solver_base.h"

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <utility>

template<typename Evaluator>
class SimpleSolver : public SolverBase<Evaluator>
{
public:
    explicit SimpleSolver(const TimeTableProblem& problem)
        : SolverBase<Evaluator>(problem) {}

    std::vector<TimeTableState> solve(int max_solutions = 10) override;
};

// Implementation lives here because SimpleSolver is a template.
// Explicit instantiations for known evaluators are in simple_solver.cpp.
template<typename Evaluator>
std::vector<TimeTableState> SimpleSolver<Evaluator>::solve(int max_solutions)
{
    // Build per-subject group list: subject_id -> [group0, group1, ...]
    const auto& classes = this->problem_.get_classes();

    std::vector<int> subject_ids;
    std::unordered_map<int, std::vector<int>> subject_groups;

    for (const auto& c : classes)
    {
        auto [it, inserted] = subject_groups.emplace(c.id, std::vector<int>{});
        if (inserted)
            subject_ids.push_back(c.id);
        it->second.push_back(c.group);
    }

    // Enumerate every combination (Cartesian product of groups per subject),
    // pruning with has_conflict and scoring complete states.
    using ScoredState = std::pair<double, TimeTableState>;
    std::vector<ScoredState> all_solutions;

    const int n = static_cast<int>(subject_ids.size());
    TimeTableState current;

    std::function<void(int)> backtrack = [&](int depth)
    {
        if (depth == n)
        {
            all_solutions.emplace_back(this->evaluator_.score(current), current);
            return;
        }

        const int class_id = subject_ids[depth];
        for (const int group : subject_groups.at(class_id))
        {
            current.add(class_id, group);
            if (!this->evaluator_.has_conflict(class_id, current))
                backtrack(depth + 1);
            current.remove(class_id);
        }
    };

    backtrack(0);

    // Sort ascending — lower score is better.
    std::sort(all_solutions.begin(), all_solutions.end(),
              [](const ScoredState& a, const ScoredState& b) {
                  return a.first < b.first;
              });

    const int n_out = std::min(max_solutions, static_cast<int>(all_solutions.size()));
    std::vector<TimeTableState> result;
    result.reserve(n_out);
    for (int i = 0; i < n_out; ++i)
        result.push_back(std::move(all_solutions[i].second));

    return result;
}

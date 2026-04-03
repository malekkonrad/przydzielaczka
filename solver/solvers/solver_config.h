//
// Created by mateu on 03.04.2026.
//

#pragma once

namespace solver
{
    struct config
    {
        size_t max_keep_solutions = 10000;
        size_t max_solutions = 10;
        size_t max_runtime = 10000; // time in seconds
        bool verbose = false;
        std::function<void(const TimeTableState& state)> intermediate_solution_callback = [](const TimeTableState& state){};
    };
} // namespace solver
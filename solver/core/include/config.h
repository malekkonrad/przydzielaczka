//
// Created by mateu on 03.04.2026.
//

#pragma once

#include <ostream>
#include <functional>

class TimeTableState;

namespace input_models
{
    struct config
    {
        size_t max_keep_solutions = 10000;
        size_t max_solutions = 10;
        size_t max_runtime = 10000; // time in seconds
        bool verbose = false;
        bool early_stopping = false;
        bool simplified_evaluation = false;
        std::function<void(const TimeTableState& state)> intermediate_solution_callback = [](const TimeTableState& state){};
    };
    inline std::ostream& operator<<(std::ostream& os, const config& c)
    {
        os << "max_keep_solutions: " << c.max_keep_solutions << "\n"
           << "max_solutions:      " << c.max_solutions << "\n"
           << "max_runtime:        " << c.max_runtime << "s\n"
           << "verbose:            " << std::boolalpha << c.verbose << "\n"
           << "early_stopping:     " << c.early_stopping;
        return os;
    }
} // namespace solver
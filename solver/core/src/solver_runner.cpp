//
// Created by mateu on 31.03.2026.
//

#include "solver_runner.h"
#include "constraints.h"
#include "input_data_mapper.h"
#include "solver.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <sstream>

#include <nlohmann/json.hpp>

using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// -------------------- CONSTRUCTORS --------------------

SolverRunner::SolverRunner(int max_solutions)
    : max_solutions_(max_solutions) {}

// -------------------- PUBLIC --------------------

Json SolverRunner::run(const Json& input, bool verbose) const
{
    InputDataMapper mapper(input);
    TimeTableProblem problem = mapper.get_problem();

    const int n_classes     = static_cast<int>(problem.get_classes().size());
    const int n_constraints = static_cast<int>(problem.get_constraints().size());

    Solver solver(problem);
    if (verbose)
        std::cout << solver << "\n";

    const auto t_start = Clock::now();
    std::vector<TimeTableState> solutions = solver.solve(max_solutions_);
    const auto t_end = Clock::now();

    const long long duration_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start).count();

    if (verbose)
    {
        for (int i = 0; i < static_cast<int>(solutions.size()); ++i)
        {
            const double score = constraints::evaluate_all(problem, solutions[i]);
            std::cout << "\n=== Solution " << (i + 1) << " / " << solutions.size()
                      << "  (score: " << score << ") ===\n";
            mapper.print_timetable(solutions[i]);
        }
        std::cout << "\nTotal: " << solutions.size() << " solution(s) found in "
                  << duration_ms << " ms\n";
    }

    Json result         = mapper.get_solution(problem, solutions);
    result["meta"]      = build_meta(duration_ms, n_classes, n_constraints);

    return result;
}

Json SolverRunner::run(const std::filesystem::path& input_path, bool verbose) const
{
    std::ifstream file(input_path);
    if (!file.is_open())
        return Json{{"error", "Could not open file: " + input_path.string()}};
    return run(Json::parse(file), verbose);
}

Json SolverRunner::run(const std::string& input_path, bool verbose) const
{
    return run(std::filesystem::path(input_path), verbose);
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const SolverRunner& r)
{
    out << "SolverRunner{ max_solutions=" << r.max_solutions_ << " }";
    return out;
}

// -------------------- PRIVATE --------------------

Json SolverRunner::build_meta(long long duration_ms, int n_classes, int n_constraints)
{
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&t), "%Y-%m-%dT%H:%M:%SZ");

    return Json{
        {"solver_version",          "0.0.1"},
        {"solved_at",               oss.str()},
        {"duration_ms",             duration_ms},
        {"algorithm",               "backtracking"},
        {"input_classes_total",     n_classes},
        {"input_constraints_total", n_constraints}
    };
}

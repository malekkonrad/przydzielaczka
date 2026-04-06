//
// Created by mateu on 31.03.2026.
//

#include "solver_runner.h"

#include <branch_and_bound_solver.h>

#include "constraints.h"
#include "data_mapper.h"
#include "simple_solver.h"
#include "constraint_evaluator.h"
#include "policies/int_time_policy.h"

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optimized_full_solver.h>
#include <ostream>
#include <simple_full_solver.h>
#include <sstream>

#include <nlohmann/json.hpp>

using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;

// -------------------- CONSTRUCTORS --------------------

SolverRunner::SolverRunner(const int max_solutions)
    : max_solutions_(max_solutions)
{
}

// -------------------- PUBLIC --------------------

Json SolverRunner::run(const Json& input, const bool verbose) const
{
    DataMapper mapper(input);
    const TimeTableProblem& problem = mapper.get_problem();

    const int n_classes     = static_cast<int>(problem.get_classes().size());
    const int n_constraints = static_cast<int>(problem.get_constraints().size());

    solver::config config;
    config.max_solutions = static_cast<size_t>(max_solutions_);
    config.verbose = true;
    // TODO add some selector
    // SimpleSolver<ConstraintEvaluator<IntTimePolicy>> solver(problem, config);
    // SimpleFullSolver solver(problem, config);
    OptimizedFullSolver<PolicyEvaluator<>> solver(problem, config);
    // BranchAndBoundSolver solver(problem, config);

    const auto t_start = Clock::now();
    const std::vector<TimeTableState> solutions = solver.solve();
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

    Json result         = mapper.get_solution(solutions);
    result["meta"]      = build_meta(duration_ms, n_classes, n_constraints);

    return result;
}

Json SolverRunner::run(const std::filesystem::path& input_path, const bool verbose) const
{
    std::ifstream file(input_path);
    if (!file.is_open())
        return Json{{"error", "Could not open file: " + input_path.string()}};
    return run(Json::parse(file), verbose);
}

Json SolverRunner::run(const std::string& input_path, const bool verbose) const
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

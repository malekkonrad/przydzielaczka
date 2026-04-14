//
// Created by mateu on 30.03.2026.
//

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "solver_config.h"
#include "data_mapper.h"
#include "solver_runner.h"

void test_print(const std::string& input_file_name)
{
    auto mapper = DataMapper(input_file_name);
    const TimeTableProblem& problem = mapper.get_problem();

    auto state = TimeTableState{problem.class_size()};
    for (int i = 0; i < state.size(); i++)
    {
        state.attend(i, 0);
    }

    mapper.print_timetable(state);
}

void test_simple_solver(const std::string& input_file_name)
{
    // Use path from command line, or fall back to the bundled sample file.

    std::cout << "Input: " << input_file_name << "\n";

    solver::config config;
    config.max_solutions = 5;
    config.verbose = true;
    config.early_stopping = false;
    const SolverRunner runner(config);
    std::cout << runner << "\n\n";

    // Run the full pipeline with verbose output:
    //   - prints Solver infoNo
    //   - prints an ASCII timetable for every solution found
    //   - prints timing summary
    const nlohmann::json result = runner.run(input_file_name, /*verbose=*/true);

    // std::cout << "\n--- JSON result ---\n"
    //           << result.dump(2) << "\n";
}

int main(int argc, char* argv[])
{
    const std::string input_file_name = "../../../../tests/data/classes_2526_L_ISI.json";
    // const std::string input_file_name = "../../../../tests/data/classes_2526_L_ISI_EIT.json";
    // const std::string input_file_name = "../../../../tests/data/classes_2526_test.json";
    // const std::string input_file_name = "../../../../tests/data/classes_2526_L_ISI_complex_test.json";

    // test_print(input_file_name);
    test_simple_solver(input_file_name);

    return 0;
}

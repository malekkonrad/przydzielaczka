//
// Created by mateu on 30.03.2026.
//

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "data_mapper.h"
#include "solver_runner.h"

int main(int argc, char* argv[])
{
    auto mapper = DataMapper("../../../../tests/data/classes_2526_L_ISI.json");
    mapper.get_problem();

    auto state = TimeTableState();
    // state.add(0, 0);
    // state.add(0, 1);
    state.add(0, 2);
    state.add(0, 3);

    mapper.print_timetable(state);

    // // Use path from command line, or fall back to the bundled sample file.
    // const std::string input_path = (argc > 1) ? argv[1] : SAMPLE_INPUT_PATH;
    //
    // std::cout << "Input: " << input_path << "\n";
    //
    // SolverRunner runner(5);
    // std::cout << runner << "\n\n";
    //
    // // Run the full pipeline with verbose output:
    // //   - prints Solver info
    // //   - prints an ASCII timetable for every solution found
    // //   - prints timing summary
    // const nlohmann::json result = runner.run(input_path, /*verbose=*/true);
    //
    // std::cout << "\n--- JSON result ---\n"
    //           << result.dump(2) << "\n";

    return 0;
}

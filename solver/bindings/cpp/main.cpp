//
// Created by mateu on 30.03.2026.
//

#include <iostream>
#include <string>

#include <nlohmann/json.hpp>

#include "solver_runner.h"

int main(int argc, char* argv[])
{
    // Use path from command line, or fall back to the bundled sample file.
    const std::string input_path = (argc > 1) ? argv[1] : SAMPLE_INPUT_PATH;

    std::cout << "Input: " << input_path << "\n";

    SolverRunner runner(5);
    std::cout << runner << "\n\n";

    // Run the full pipeline with verbose output:
    //   - prints Solver info
    //   - prints an ASCII timetable for every solution found
    //   - prints timing summary
    const nlohmann::json result = runner.run(input_path, /*verbose=*/true);

    std::cout << "\n--- JSON result ---\n"
              << result.dump(2) << "\n";

    return 0;
}

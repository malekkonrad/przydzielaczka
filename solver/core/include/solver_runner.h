//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <filesystem>
#include <iosfwd>
#include <string>

#include <nlohmann/json_fwd.hpp>

using Json = nlohmann::json;

// Executes the full solve pipeline:
//   parse input → build problem → run solver → format output
//
// Usage:
//   SolverRunner runner(10);
//   Json result = runner.run(input_json);        // silent
//   Json result = runner.run(input_json, true);  // verbose: prints each solution's timetable
class SolverRunner
{
public:
    explicit SolverRunner(int max_solutions = 10);

    [[nodiscard]] Json run(const Json& input, bool verbose = false) const;
    [[nodiscard]] Json run(const std::filesystem::path& input_path, bool verbose = false) const;
    [[nodiscard]] Json run(const std::string& input_path, bool verbose = false) const;

    friend std::ostream& operator<<(std::ostream& out, const SolverRunner& r);

private:
    int max_solutions_;

    // Builds the "meta" object added to the output JSON.
    [[nodiscard]] static Json build_meta(
        long long duration_ms,
        int n_classes,
        int n_constraints
    );
};

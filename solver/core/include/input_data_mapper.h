//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <time_table_problem.h>
#include <input_data_models.h>
#include <time_table_state.h>

#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <iostream>
#include <vector>

using Json = nlohmann::json;

// ----------- INPUT DATA MAPPER -------------

class InputDataMapper
{
public:
	InputDataMapper();
	explicit InputDataMapper(const Json& data);
	explicit InputDataMapper(const std::string& input_file);
	explicit InputDataMapper(const std::filesystem::path& input_path);
	explicit InputDataMapper(const TimeTableProblem& problem);

	InputDataMapper(const InputDataMapper& other);
	InputDataMapper& operator=(const InputDataMapper& other);
	InputDataMapper(InputDataMapper&& other) noexcept;
	InputDataMapper& operator=(InputDataMapper&& other) noexcept;

	InputDataMapper& parse(const Json& data);
	InputDataMapper& parse(const std::string& input_file);
	InputDataMapper& parse(const std::filesystem::path& input_path);
	[[nodiscard]] TimeTableProblem get_problem();

	InputDataMapper& parse(const TimeTableProblem& problem);
	[[nodiscard]] Json get_solution(const TimeTableProblem& problem,
	                                const std::vector<TimeTableState>& solutions);
	[[nodiscard]] Json get_solution() const;

	// Prints a two-week (A/B) ASCII timetable for the given solution to `out`.
	void print_timetable(const TimeTableState& state, std::ostream& out = std::cout) const;

	[[nodiscard]] Json to_json() const;
	[[nodiscard]] static InputDataMapper from_json(const Json& data);

	friend std::ostream& operator<<(std::ostream& out, const InputDataMapper& m);

private:
	[[nodiscard]] bool validate(const Json& data) const;

	std::optional<input_models::Timetable> timetable_ = std::nullopt;
	std::optional<TimeTableProblem> problem_ = std::nullopt;
	std::vector<TimeTableState> solutions_;
};
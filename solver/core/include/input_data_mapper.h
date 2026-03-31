//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <time_table_problem.h>
#include <input_data_models.h>

#include <nlohmann/json_fwd.hpp>

#include <filesystem>

class TimeTableState;

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
	[[nodiscard]] Json get_solution(const TimeTableProblem& problem);
	[[nodiscard]] Json get_solution() const;

	[[nodiscard]] Json to_json() const;
	[[nodiscard]] static InputDataMapper from_json(const Json& data);

private:
	[[nodiscard]] bool validate(const Json& data) const;

	std::optional<input_models::Timetable> timetable_ = std::nullopt;
	std::optional<TimeTableProblem> problem_ = std::nullopt;
};
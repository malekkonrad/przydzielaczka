//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <time_table_problem.h>
#include <data_models.h>
#include <time_table_state.h>

#include <nlohmann/json_fwd.hpp>

#include <tuple_hash.h>

#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <tuple>

using Json = nlohmann::json;

// ----------- DATA MAPPER -------------

class DataMapper
{
public:
	DataMapper();
	explicit DataMapper(const Json& data);
	explicit DataMapper(const std::string& input_file);
	explicit DataMapper(const std::filesystem::path& input_path);
	explicit DataMapper(const char* input_path);

	DataMapper(const DataMapper& other);
	DataMapper& operator=(const DataMapper& other);
	DataMapper(DataMapper&& other) noexcept;
	DataMapper& operator=(DataMapper&& other) noexcept;
	~DataMapper() = default;

	DataMapper& parse(const Json& data);
	DataMapper& parse(const std::string& input_file);
	DataMapper& parse(const std::filesystem::path& input_path);
	DataMapper& parse(const char* input_file);
	const TimeTableProblem& get_problem();

	[[nodiscard]] Json get_solution(const std::vector<TimeTableState>& solutions);
	[[nodiscard]] Json get_solution() const;

	// Prints a two-week (A/B) ASCII timetable for the given solution to `out`.
	void print_timetable(const TimeTableState& state, std::ostream& out = std::cout) const;

	friend std::ostream& operator<<(std::ostream& out, const DataMapper& data_mapper);

private:
	[[nodiscard]] static bool validate(const Json& data);

	// Mappers: domain value -> solver int id
	[[nodiscard]] TimeTableProblem map_problem();
	[[nodiscard]] std::vector<solver_models::Class> map_classes();
	[[nodiscard]] std::vector<solver_models::ConstraintVariant> map_constraints() const;
	[[nodiscard]] int map_class_id_and_class_type(const std::string& class_id, const std::string& class_type);
	[[nodiscard]] int map_group(int class_id, int group);
	[[nodiscard]] int map_date(const std::string& date);
	[[nodiscard]] int map_day(int day);
	[[nodiscard]] int map_time(int time);
	[[nodiscard]] std::bitset<2> map_week(const std::string& week);
	[[nodiscard]] int map_location(const input_models::Location& location);
	[[nodiscard]] int map_lecturer(const std::string& lecturer);

	// Demappers: solver int id -> domain value
	[[nodiscard]] std::tuple<std::string, std::string> demap_class_id_and_class_type(int id) const;

	// Helpers
	[[nodiscard]] std::optional<int> find_class_id_and_class_type(const std::string& class_id, const std::string& class_type) const;
	[[nodiscard]] std::optional<int> find_group(int class_id, int group) const;
	[[nodiscard]] std::optional<int> find_lecturer(const std::string& lecturer) const;
	[[nodiscard]] std::optional<int> find_day(int day) const;
	[[nodiscard]] std::optional<int> find_date(const std::string& date) const;
	[[nodiscard]] std::optional<int> find_time(int time) const;

	std::optional<input_models::Timetable> timetable_ = std::nullopt;
	std::optional<TimeTableProblem> problem_ = std::nullopt;
	std::vector<TimeTableState> solutions_;

	// Forward maps (mappers)
	std::unordered_map<std::tuple<std::string, std::string>, int> class_id_mapper_;
	std::unordered_map<int, std::unordered_map<int, int>> group_mapper_;
	std::unordered_map<int, int> day_mapper_;
	std::unordered_map<std::string, int> date_mapper_;
	std::unordered_map<std::tuple<std::string, std::string>, int> location_mapper_;
	std::unordered_map<std::string, int> lecturer_mapper_;

	// Reverse maps (demappers)
	std::unordered_map<int, std::tuple<std::string, std::string>> class_id_demapper_;
	std::unordered_map<int, std::unordered_map<int, int>> group_demapper_;
};
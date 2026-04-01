//
// Created by mateu on 30.03.2026.
//

#pragma once

#include <time_table_problem.h>
#include <data_models.h>
#include <time_table_state.h>

#include <nlohmann/json_fwd.hpp>

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
	explicit DataMapper(const TimeTableProblem& problem);

	DataMapper(const DataMapper& other);
	DataMapper& operator=(const DataMapper& other);
	DataMapper(DataMapper&& other) noexcept;
	DataMapper& operator=(DataMapper&& other) noexcept;
	~DataMapper() = default;

	DataMapper& parse(const Json& data);
	DataMapper& parse(const std::string& input_file);
	DataMapper& parse(const std::filesystem::path& input_path);
	[[nodiscard]] const TimeTableProblem& get_problem();

	DataMapper& parse(const TimeTableProblem& problem);
	[[nodiscard]] Json get_solution(const std::vector<TimeTableState>& solutions);
	[[nodiscard]] Json get_solution() const;

	// Prints a two-week (A/B) ASCII timetable for the given solution to `out`.
	void print_timetable(const TimeTableState& state, std::ostream& out = std::cout) const;

	friend std::ostream& operator<<(std::ostream& out, const DataMapper& m);

private:
	[[nodiscard]] static bool validate(const Json& data);

	// Mappers: domain value → solver int id
	[[nodiscard]] std::vector<solver_models::Class> map_classes();
	[[nodiscard]] std::vector<solver_models::ConstraintVariant> map_constraints();
	[[nodiscard]] int map_class_id_and_class_type(const std::string& class_id, const std::string& class_type);
	[[nodiscard]] int map_date(const std::string& date);
	[[nodiscard]] int map_day(int day);
	[[nodiscard]] int map_time(int time);
	[[nodiscard]] std::bitset<2> map_week(const std::string& week);
	[[nodiscard]] int map_location(const input_models::Location& location);
	[[nodiscard]] int map_lecturer(const std::string& lecturer);

	// Demappers: solver int id → domain value
	[[nodiscard]] std::pair<std::string, std::string> demap_class_id_and_class_type(int id) const;
	[[nodiscard]] std::string demap_date(int date) const;
	[[nodiscard]] int demap_day(int day) const;
	[[nodiscard]] int demap_time(int time) const;
	[[nodiscard]] std::string demap_week(std::bitset<2> week) const;
	[[nodiscard]] input_models::Location demap_location(int location) const;
	[[nodiscard]] std::string demap_lecturer(int lecturer) const;

	// Helper: all int ids whose string class_id matches (any class_type)
	[[nodiscard]] std::vector<int> find_class_int_ids(const std::string& class_id_str) const;

	std::optional<input_models::Timetable> timetable_ = std::nullopt;
	std::optional<TimeTableProblem> problem_ = std::nullopt;
	std::vector<TimeTableState> solutions_;

	// Forward maps (mappers)
	std::unordered_map<std::tuple<std::string, std::string>, int> class_id_mapper_;
	std::unordered_map<std::string, int> date_mapper_;
	std::unordered_map<std::string, std::bitset<2>> week_mapper_;
	std::unordered_map<std::string, int> location_mapper_;
	std::unordered_map<std::string, int> lecturer_mapper_;

	// Reverse maps (demappers)
	std::unordered_map<int, std::tuple<std::string, std::string>> class_id_demapper_;
	std::unordered_map<int, std::string> date_demapper_;
	std::unordered_map<int, input_models::Location> location_demapper_;
	std::unordered_map<int, std::string> lecturer_demapper_;
};
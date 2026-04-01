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
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using Json = nlohmann::json;

// Hash for unordered_map keyed on pair<string,string>
template<>
struct std::hash<std::pair<std::string, std::string>>
{
	std::size_t operator()(const std::pair<std::string, std::string>& p) const noexcept
	{
		std::size_t h1 = std::hash<std::string>{}(p.first);
		std::size_t h2 = std::hash<std::string>{}(p.second);
		return h1 ^ (h2 * 2654435761ULL); // Knuth multiplicative hash mix
	}
}; // namespace std

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
	[[nodiscard]] const TimeTableProblem& get_problem();

	InputDataMapper& parse(const TimeTableProblem& problem);
	[[nodiscard]] Json get_solution(const std::vector<TimeTableState>& solutions);
	[[nodiscard]] Json get_solution() const;

	// Prints a two-week (A/B) ASCII timetable for the given solution to `out`.
	void print_timetable(const TimeTableState& state, std::ostream& out = std::cout) const;

	friend std::ostream& operator<<(std::ostream& out, const InputDataMapper& m);

private:
	[[nodiscard]] static bool validate(const Json& data);

	// Mappers: domain value → solver int id
	[[nodiscard]] std::vector<solver_models::Class> map_classes();
	[[nodiscard]] std::vector<solver_models::ConstraintVariant> map_constraints();
	[[nodiscard]] int map_class_id_and_class_type(const std::string& class_id, const std::string& class_type);
	[[nodiscard]] solver_models::TimeTableDate map_date(const std::string& date);
	[[nodiscard]] solver_models::TimeTableDay  map_day(int day);
	[[nodiscard]] solver_models::TimeTableTime map_time(int time);
	[[nodiscard]] solver_models::TimeTableWeek map_week(const std::string& week);
	[[nodiscard]] int map_location(const input_models::Location& location);
	[[nodiscard]] int map_lecturer(const std::string& lecturer);

	// Demappers: solver int id → domain value
	[[nodiscard]] std::pair<std::string, std::string> demap_class_id_and_class_type(int id) const;
	[[nodiscard]] std::string              demap_date(solver_models::TimeTableDate date) const;
	[[nodiscard]] int                      demap_day(solver_models::TimeTableDay day) const;
	[[nodiscard]] int                      demap_time(solver_models::TimeTableTime time) const;
	[[nodiscard]] std::string              demap_week(solver_models::TimeTableWeek week) const;
	[[nodiscard]] input_models::Location   demap_location(int location) const;
	[[nodiscard]] std::string              demap_lecturer(int lecturer) const;

	// Helper: all int ids whose string class_id matches (any class_type)
	[[nodiscard]] std::vector<int> find_class_int_ids(const std::string& class_id_str) const;

	std::optional<input_models::Timetable> timetable_ = std::nullopt;
	std::optional<TimeTableProblem> problem_ = std::nullopt;
	std::vector<TimeTableState> solutions_;

	// Forward maps
	std::unordered_map<std::pair<std::string, std::string>, int> class_id_mapper_;
	std::unordered_map<std::string, solver_models::TimeTableDate> date_mapper_;
	std::unordered_map<std::string, solver_models::TimeTableWeek> week_mapper_;
	std::unordered_map<std::string, int> location_mapper_;
	std::unordered_map<std::string, int> lecturer_mapper_;

	// Reverse maps (demappers)
	std::unordered_map<int, std::pair<std::string, std::string>> class_id_demapper_;
	std::unordered_map<int, std::string>            date_demapper_;
	std::unordered_map<int, input_models::Location> location_demapper_;
	std::unordered_map<int, std::string>            lecturer_demapper_;
};
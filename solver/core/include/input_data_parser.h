//
// Created by mateu on 30.03.2026.
//

#pragma once

using json = nlohmann::json;

// -------------------- Models --------------------

struct Location {
    std::string room;
    std::string building;
};

struct Session {
    std::string date;
    Location location;
    int start_time;
    int end_time;
};

struct Class {
    std::string id;
    std::string lecturer;
    int day;
    std::string week;
    Location location;
    int group;
    std::string class_type;
    int start_time;
    int end_time;
    std::vector<Session> sessions;
};

struct Constraint {
    std::string type;
    int sequence;

    std::optional<double> weight;
    std::optional<bool> hard;
    std::optional<std::string> class_id;

    std::optional<int> min_break_duration;
    std::optional<int> preferred_group;
    std::optional<std::string> preferred_lecturer;

    std::optional<int> day;
    std::optional<std::string> date;

    std::optional<int> start_time;
    std::optional<int> end_time;

    std::optional<std::string> position;
};

struct Timetable {
    std::vector<Constraint> constraints;
    std::vector<Class> classes;
};

class InputDataParser {
private:
public:
	// Constructors
	InputDataParser();
	InputDataParser(json data);
	InputDataParser(std::string input_file);
	InputDataParser(std::filesystem::path input_path);

	InputDataParser(const InputDataParser& input_data_parser);
	InputDataParser(InputDataParser&& input_data_parser) noexcept;

	InputDataParser& operator=(const InputDataParser& input_data_parser);
	InputDataParser& operator=(InputDataParser&& input_data_parser) noexcept;

	// Methods
	InputDataParser& parse(json data);
	InputDataParser& parse(std::filesystem::path input_path);
	InputDataParser& parse(std::string input_file);
	TimetableProblem to_problem();
};
//
// Created by mateu on 30.03.2026.
//

#include "input_data_parser.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <optional>
#include <nlohmann/json.hpp>

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

// -------------------- Validation Helpers --------------------

void validate_time(int t) {
    if (t < 0 || t > 1440)
        throw std::runtime_error("Invalid time: must be 0-1440");
}

void validate_day(int d) {
    if (d < 0 || d > 6)
        throw std::runtime_error("Invalid day: must be 0-6");
}

void validate_week(const std::string& w) {
    if (w != "A" && w != "B" && w != "AB")
        throw std::runtime_error("Invalid week value");
}

// -------------------- Parsing --------------------

Location parse_location(const json& j) {
    return {
        j.at("room").get<std::string>(),
        j.at("building").get<std::string>()
    };
}

Session parse_session(const json& j) {
    Session s;
    s.date = j.at("date").get<std::string>();
    s.location = parse_location(j.at("location"));
    s.start_time = j.at("start_time").get<int>();
    s.end_time = j.at("end_time").get<int>();

    validate_time(s.start_time);
    validate_time(s.end_time);

    return s;
}

Class parse_class(const json& j) {
    Class c;
    c.id = j.at("id").get<std::string>();
    c.lecturer = j.at("lecturer").get<std::string>();
    c.day = j.at("day").get<int>();
    c.week = j.at("week").get<std::string>();
    c.location = parse_location(j.at("location"));
    c.group = j.at("group").get<int>();
    c.class_type = j.at("class_type").get<std::string>();
    c.start_time = j.at("start_time").get<int>();
    c.end_time = j.at("end_time").get<int>();

    validate_day(c.day);
    validate_week(c.week);
    validate_time(c.start_time);
    validate_time(c.end_time);

    for (const auto& s : j.at("sessions")) {
        c.sessions.push_back(parse_session(s));
    }

    return c;
}

Constraint parse_constraint(const json& j) {
    Constraint c;

    c.type = j.at("constraint_type").get<std::string>();
    c.sequence = j.at("sequence").get<int>();

    if (j.contains("weight")) c.weight = j["weight"].get<double>();
    if (j.contains("hard")) c.hard = j["hard"].get<bool>();
    if (j.contains("class_id")) c.class_id = j["class_id"].get<std::string>();

    if (j.contains("min_break_duration"))
        c.min_break_duration = j["min_break_duration"].get<int>();

    if (j.contains("preferred_group"))
        c.preferred_group = j["preferred_group"].get<int>();

    if (j.contains("preferred_lecturer"))
        c.preferred_lecturer = j["preferred_lecturer"].get<std::string>();

    if (j.contains("day")) {
        c.day = j["day"].get<int>();
        validate_day(*c.day);
    }

    if (j.contains("date"))
        c.date = j["date"].get<std::string>();

    if (j.contains("start_time")) {
        c.start_time = j["start_time"].get<int>();
        validate_time(*c.start_time);
    }

    if (j.contains("end_time")) {
        c.end_time = j["end_time"].get<int>();
        validate_time(*c.end_time);
    }

    if (j.contains("position"))
        c.position = j["position"].get<std::string>();

    // ----------- LOGIC VALIDATION (schema-like) -----------

    if (c.type == "minimize_gaps") {
        if (!c.min_break_duration)
            throw std::runtime_error("minimize_gaps requires min_break_duration");
    }

    if (c.type == "group_preference") {
        if (!c.class_id || !c.preferred_group)
            throw std::runtime_error("group_preference requires class_id and preferred_group");
    }

    if (c.type == "lecturer_preference") {
        if (!c.class_id || !c.preferred_lecturer)
            throw std::runtime_error("lecturer_preference requires class_id and preferred_lecturer");
    }

    if (c.type == "maximize_single_attendance") {
        if (!c.class_id)
            throw std::runtime_error("maximize_single_attendance requires class_id");
    }

    if (c.type == "time_block") {
        if (!c.start_time || !c.end_time)
            throw std::runtime_error("time_block requires start_time and end_time");

        if (!c.day && !c.date)
            throw std::runtime_error("time_block requires day or date");
    }

    if (c.type == "prefer_edge_classes") {
        if (!c.class_id || !c.position)
            throw std::runtime_error("prefer_edge_classes requires class_id and position");
    }

    return c;
}

Timetable parse_timetable(const json& j) {
    Timetable t;

    for (const auto& c : j.at("constraints")) {
        t.constraints.push_back(parse_constraint(c));
    }

    for (const auto& c : j.at("classes")) {
        t.classes.push_back(parse_class(c));
    }

    return t;
}

// -------------------- MAIN --------------------

int main() {
    std::ifstream file("input.json");
    json j;
    file >> j;

    try {
        Timetable t = parse_timetable(j);
        std::cout << "Parsed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "Validation error: " << e.what() << "\n";
    }

    return 0;
}
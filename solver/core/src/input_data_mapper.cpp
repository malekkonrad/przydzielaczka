//
// Created by mateu on 30.03.2026.
//

#include "input_data_mapper.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json-schema.hpp>

using Json = nlohmann::json;

// -------------------- PARSE HELPERS --------------------

template<typename T>
static std::optional<T> get_opt(const Json& j, const std::string& key)
{
    if (j.contains(key) && !j.at(key).is_null())
        return j.at(key).get<T>();
    return std::nullopt;
}

static input_models::Location parse_location(const Json& j)
{
    return input_models::Location{
        j.at("room").get<std::string>(),
        j.at("building").get<std::string>()
    };
}

static input_models::Session parse_session(const Json& j)
{
    return input_models::Session{
        j.at("date").get<std::string>(),
        parse_location(j.at("location")),
        j.at("start_time").get<int>(),
        j.at("end_time").get<int>()
    };
}

static input_models::Class parse_class(const Json& j)
{
    input_models::Class c;
    c.id = j.at("id").get<std::string>();
    c.lecturer = j.at("lecturer").get<std::string>();
    c.day = j.at("day").get<int>();
    c.week = j.at("week").get<std::string>();
    c.location = parse_location(j.at("location"));
    c.group = j.at("group").get<int>();
    c.class_type = j.at("class_type").get<std::string>();
    c.start_time = j.at("start_time").get<int>();
    c.end_time = j.at("end_time").get<int>();
    for (const auto& s : j.at("sessions"))
        c.sessions.push_back(parse_session(s));
    return c;
}

static input_models::Constraint parse_constraint(const Json& j)
{
    input_models::Constraint c;
    c.type = j.at("constraint_type").get<std::string>();
    c.sequence = j.at("sequence").get<int>();
    c.weight = get_opt<double>(j, "weight");
    c.hard = get_opt<bool>(j, "hard");
    c.class_id = get_opt<std::string>(j, "class_id");
    c.min_break_duration = get_opt<int>(j, "min_break_duration");
    c.preferred_group = get_opt<int>(j, "preferred_group");
    c.preferred_lecturer = get_opt<std::string>(j, "preferred_lecturer");
    c.day = get_opt<int>(j, "day");
    c.date = get_opt<std::string>(j, "date");
    c.start_time = get_opt<int>(j, "start_time");
    c.end_time = get_opt<int>(j, "end_time");
    c.position = get_opt<std::string>(j, "position");
    return c;
}

static input_models::Timetable parse_timetable(const Json& j)
{
    input_models::Timetable t;
    for (const auto& c : j.at("constraints")) t.constraints.push_back(parse_constraint(c));
    for (const auto& cl : j.at("classes")) t.classes.push_back(parse_class(cl));
    return t;
}

// -------------------- SERIALIZATION HELPERS --------------------

static Json location_to_json(const input_models::Location& loc)
{
    return Json{{"room", loc.room}, {"building", loc.building}};
}

static Json session_to_json(const input_models::Session& s)
{
    return Json{
        {"date", s.date},
        {"location", location_to_json(s.location)},
        {"start_time", s.start_time},
        {"end_time", s.end_time}
    };
}

static Json class_to_json(const input_models::Class& c)
{
    Json sessions = Json::array();
    for (const auto& s : c.sessions)
        sessions.push_back(session_to_json(s));

    return Json{
        {"id", c.id},
        {"lecturer", c.lecturer},
        {"day", c.day},
        {"week", c.week},
        {"location", location_to_json(c.location)},
        {"group", c.group},
        {"class_type", c.class_type},
        {"start_time", c.start_time},
        {"end_time", c.end_time},
        {"sessions", sessions}
    };
}

static Json constraint_to_json(const input_models::Constraint& c)
{
    Json j;
    j["constraint_type"] = c.type;
    j["sequence"] = c.sequence;
    if (c.weight) j["weight"] = *c.weight;
    if (c.hard) j["hard"] = *c.hard;
    if (c.class_id) j["class_id"] = *c.class_id;
    if (c.min_break_duration) j["min_break_duration"] = *c.min_break_duration;
    if (c.preferred_group) j["preferred_group"] = *c.preferred_group;
    if (c.preferred_lecturer) j["preferred_lecturer"] = *c.preferred_lecturer;
    if (c.day) j["day"] = *c.day;
    if (c.date) j["date"] = *c.date;
    if (c.start_time) j["start_time"] = *c.start_time;
    if (c.end_time) j["end_time"] = *c.end_time;
    if (c.position) j["position"] = *c.position;
    return j;
}

// -------------------- CONSTRUCTORS --------------------

InputDataMapper::InputDataMapper() = default;

InputDataMapper::InputDataMapper(const Json& data)
{
    parse(data);
}

InputDataMapper::InputDataMapper(const std::string& input_file)
{
    parse(input_file);
}

InputDataMapper::InputDataMapper(const std::filesystem::path& input_path)
{
    parse(input_path);
}

InputDataMapper::InputDataMapper(const TimeTableProblem& problem)
{
    parse(problem);
}

InputDataMapper::InputDataMapper(const InputDataMapper& other)
    : timetable_(other.timetable_), problem_(other.problem_)
{
}

InputDataMapper& InputDataMapper::operator=(const InputDataMapper& other)
{
    if (this != &other)
    {
        timetable_ = other.timetable_;
        problem_ = other.problem_;
    }
    return *this;
}

InputDataMapper::InputDataMapper(InputDataMapper&& other) noexcept
    : timetable_(std::move(other.timetable_)), problem_(std::move(other.problem_))
{
}

InputDataMapper& InputDataMapper::operator=(InputDataMapper&& other) noexcept
{
    if (this != &other)
    {
        timetable_ = std::move(other.timetable_);
        problem_ = std::move(other.problem_);
    }
    return *this;
}

// -------------------- VALIDATION --------------------

bool InputDataMapper::validate(const Json& data) const
{
    static const Json schema = []() -> Json
    {
        std::ifstream file(INPUT_SCHEMA_PATH);
        if (!file.is_open())
        {
            std::cerr << "InputDataMapper: could not open schema: " << INPUT_SCHEMA_PATH << '\n';
            return Json{};
        }
        return Json::parse(file);
    }();

    if (schema.empty())
        return false;

    nlohmann::json_schema::json_validator validator;
    try
    {
        validator.set_root_schema(schema);
        validator.validate(data);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "InputDataMapper: validation failed: " << e.what() << '\n';
        return false;
    }
}

// -------------------- PARSE: JSON → Timetable --------------------

InputDataMapper& InputDataMapper::parse(const Json& data)
{
    if (!validate(data))
        return *this;

    timetable_ = parse_timetable(data);
    return *this;
}

InputDataMapper& InputDataMapper::parse(const std::string& input_file)
{
    return parse(std::filesystem::path(input_file));
}

InputDataMapper& InputDataMapper::parse(const std::filesystem::path& input_path)
{
    std::ifstream file(input_path);
    if (!file.is_open())
    {
        std::cerr << "InputDataMapper: could not open file: " << input_path << '\n';
        return *this;
    }
    Json j;
    file >> j;
    return parse(j);
}

// -------------------- PARSE: Problem/State → Timetable --------------------

InputDataMapper& InputDataMapper::parse(const TimeTableProblem& problem)
{
    problem_ = problem;
    return *this;
}

// -------------------- GET PROBLEM --------------------

TimeTableProblem InputDataMapper::get_problem()
{
    // TimeTableProblem not yet defined
    return TimeTableProblem{};
}

// -------------------- GET SOLUTION --------------------

// Wraps a constraint's definition with placeholder result fields.
// Actual scores are filled in by the solver — the mapper only provides structure.
static Json constraint_result_to_json(const input_models::Constraint& c)
{
    Json j = constraint_to_json(c);
    j["satisfied"]      = false;
    j["score_achieved"] = 0.0;
    j["score_max"]      = c.weight.value_or(0.0);
    j["details"]        = "";
    return j;
}

Json InputDataMapper::get_solution(const TimeTableProblem& problem)
{
    problem_ = problem;
    return get_solution();
}

Json InputDataMapper::get_solution() const
{
    if (!problem_)
        return to_json();

    const auto& solutions   = problem_->get_solutions();
    const auto& all_classes = timetable_ ? timetable_->classes     : decltype(timetable_->classes){};
    const auto& constraints = timetable_ ? timetable_->constraints  : decltype(timetable_->constraints){};

    // Build one result entry per solution
    Json results = Json::array();
    for (const auto& state : solutions)
    {
        // Map chosen int IDs → input_models::Class by treating the ID as an index
        Json chosen = Json::array();
        for (const int id : state.get_chosen_ids())
        {
            if (id >= 0 && id < static_cast<int>(all_classes.size()))
                chosen.push_back(class_to_json(all_classes[id]));
        }

        Json constraint_results = Json::array();
        for (const auto& c : constraints)
            constraint_results.push_back(constraint_result_to_json(c));

        results.push_back(Json{
            {"chosen_classes",     chosen},
            {"constraint_results", constraint_results}
        });
    }

    const int n_classes     = static_cast<int>(all_classes.size());
    const int n_constraints = static_cast<int>(constraints.size());

    Json meta = Json{
        {"solver_version",          "0.0.1"},
        {"solved_at",               ""},
        {"duration_ms",             0},
        {"algorithm",               ""},
        {"input_classes_total",     n_classes},
        {"input_constraints_total", n_constraints}
    };

    Json summary = Json{
        {"feasible",                   !solutions.empty()},
        {"hard_constraints_satisfied", false},
        {"score", Json{
            {"total", 0.0},
            {"hard",  0.0},
            {"soft",  0.0}
        }}
    };

    return Json{{"meta", meta}, {"summary", summary}, {"results", results}};
}

// -------------------- JSON ROUND-TRIP --------------------

Json InputDataMapper::to_json() const
{
    if (!timetable_)
        return Json{};

    Json constraints = Json::array();
    for (const auto& c : timetable_->constraints)
        constraints.push_back(constraint_to_json(c));

    Json classes = Json::array();
    for (const auto& cl : timetable_->classes)
        classes.push_back(class_to_json(cl));

    return Json{{"constraints", constraints}, {"classes", classes}};
}

InputDataMapper InputDataMapper::from_json(const Json& data)
{
    return InputDataMapper(data);
}

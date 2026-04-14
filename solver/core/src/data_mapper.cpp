//
// Created by mateu on 30.03.2026.
//

#include <data_mapper.h>
#include <tuple_hash.h>
#include <data_models.h>
#include <constraints.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <map>
#include <ostream>
#include <set>
#include <string>
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
    c.class_type = get_opt<std::string>(j, "class_type");
    c.min_break_duration = get_opt<int>(j, "min_break_duration");
    c.group = get_opt<int>(j, "group");
    c.lecturer = get_opt<std::string>(j, "lecturer");
    c.day = get_opt<int>(j, "day");
    c.date = get_opt<std::string>(j, "date");
    c.start_time = get_opt<int>(j, "start_time");
    c.end_time = get_opt<int>(j, "end_time");
    c.position = get_opt<std::string>(j, "position");
    c.slack = get_opt<int>(j, "slack");
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
    {
        sessions.push_back(session_to_json(s));
    }

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
    if (c.weight) j["weight"] = c.weight.value();
    if (c.hard) j["hard"] = c.hard.value();
    if (c.class_id) j["class_id"] = c.class_id.value();
    if (c.class_type) j["class_type"] = c.class_type.value();
    if (c.min_break_duration) j["min_break_duration"] = c.min_break_duration.value();
    if (c.group) j["group"] = c.group.value();
    if (c.lecturer) j["lecturer"] = c.lecturer.value();
    if (c.day) j["day"] = c.day.value();
    if (c.date) j["date"] = c.date.value();
    if (c.start_time) j["start_time"] = c.start_time.value();
    if (c.end_time) j["end_time"] = c.end_time.value();
    if (c.position) j["position"] = c.position.value();
    if (c.slack) j["slack"] = c.slack.value();
    return j;
}

// -------------------- CONSTRUCTORS --------------------

DataMapper::DataMapper() = default;

DataMapper::DataMapper(const Json& data)
{
    parse(data);
}

DataMapper::DataMapper(const std::string& input_file)
{
    parse(input_file);
}

DataMapper::DataMapper(const std::filesystem::path& input_path)
{
    parse(input_path);
}

DataMapper::DataMapper(const char* input_path)
{
    parse(input_path);
}

DataMapper::DataMapper(const DataMapper& other)
    : timetable_(other.timetable_)
    , problem_(other.problem_)
    , solutions_(other.solutions_)
    , class_id_mapper_(other.class_id_mapper_)
    , date_mapper_(other.date_mapper_)
    , location_mapper_(other.location_mapper_)
    , lecturer_mapper_(other.lecturer_mapper_)
    , class_id_demapper_(other.class_id_demapper_)
{
}

DataMapper& DataMapper::operator=(const DataMapper& other)
{
    if (this != &other)
    {
        timetable_           = other.timetable_;
        problem_             = other.problem_;
        solutions_           = other.solutions_;
        class_id_mapper_     = other.class_id_mapper_;
        date_mapper_         = other.date_mapper_;
        location_mapper_     = other.location_mapper_;
        lecturer_mapper_     = other.lecturer_mapper_;
        class_id_demapper_   = other.class_id_demapper_;
    }
    return *this;
}

DataMapper::DataMapper(DataMapper&& other) noexcept
    : timetable_(std::move(other.timetable_))
    , problem_(std::move(other.problem_))
    , solutions_(std::move(other.solutions_))
    , class_id_mapper_(std::move(other.class_id_mapper_))
    , date_mapper_(std::move(other.date_mapper_))
    , location_mapper_(std::move(other.location_mapper_))
    , lecturer_mapper_(std::move(other.lecturer_mapper_))
    , class_id_demapper_(std::move(other.class_id_demapper_))
{
}

DataMapper& DataMapper::operator=(DataMapper&& other) noexcept
{
    if (this != &other)
    {
        timetable_           = std::move(other.timetable_);
        problem_             = std::move(other.problem_);
        solutions_           = std::move(other.solutions_);
        class_id_mapper_     = std::move(other.class_id_mapper_);
        date_mapper_         = std::move(other.date_mapper_);
        location_mapper_     = std::move(other.location_mapper_);
        lecturer_mapper_     = std::move(other.lecturer_mapper_);
        class_id_demapper_   = std::move(other.class_id_demapper_);
    }
    return *this;
}

// -------------------- VALIDATION --------------------

static Json load_schema()
{
    std::ifstream file(INPUT_SCHEMA_PATH);
    if (!file.is_open())
    {
        std::cerr << "InputDataMapper: could not open schema\n";
        return Json{};
    }
    return Json::parse(file); // weird clangd bug
}

bool DataMapper::validate(const Json& data)
{
    static Json schema = load_schema();

    if (schema.empty())
        return false;

    // Provide a no-op format checker so the validator does not throw when the
    // schema contains "format" keywords (e.g. "date", "date-time").
    auto format_checker = [](const std::string& /*format*/, const std::string& /*value*/) {};

    nlohmann::json_schema::json_validator validator(nullptr, format_checker);
    try
    {
        validator.set_root_schema(schema);
        validator.validate(data); // NOLINT: we just need to validate schema
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "InputDataMapper: validation failed: " << e.what() << '\n';
        return false;
    }
}

// -------------------- PARSE: JSON → Timetable --------------------

DataMapper& DataMapper::parse(const Json& data)
{
    if (!validate(data))
        return *this;

    timetable_ = parse_timetable(data);
    return *this;
}

DataMapper& DataMapper::parse(const std::string& input_file)
{
    return parse(std::filesystem::path(input_file));
}

DataMapper& DataMapper::parse(const char* input_file)
{
    return parse(std::filesystem::path(input_file));
}

DataMapper& DataMapper::parse(const std::filesystem::path& input_path)
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

// -------------------- MAPPERS --------------------

int DataMapper::map_class_id_and_class_type(const std::string& class_id,
                                            const std::string& class_type)
{
    const auto key = std::make_tuple(class_id, class_type);
    const auto it = class_id_mapper_.find(key);
    if (it != class_id_mapper_.end())
    {
        return it->second;
    }

    const int id = static_cast<int>(class_id_mapper_.size());
    class_id_mapper_[key] = id;
    class_id_demapper_[id] = key;
    return id;
}

int DataMapper::map_group(const int class_id, const int group)
{
    const auto class_it = group_mapper_.find(class_id);
    if (class_it != group_mapper_.end())
    {
        const auto& groups = class_it->second;
        const auto it = groups.find(group);
        if (it != groups.end())
        {
            return it->second;
        }
        // 1-indexed: 0 is reserved for UNASSIGNED in TimeTableState
        const auto id = static_cast<int>(groups.size()) + 1;
        group_mapper_[class_id][group] = id;
        group_demapper_[class_id][id] = group;
        return id;
    }

    // First group for this class gets id 1
    group_mapper_[class_id][group] = 1;
    group_demapper_[class_id][1] = group;
    return 1;
}

int DataMapper::map_date(const std::string& date)
{
    const auto it = date_mapper_.find(date);
    if (it != date_mapper_.end())
        return it->second;

    const int id = static_cast<int>(date_mapper_.size());
    date_mapper_[date] = id;
    return id;
}

int DataMapper::map_day(const int day)
{
    const auto it = day_mapper_.find(day);
    if (it != day_mapper_.end())
    {
        return it->second;
    }

    const int id = static_cast<int>(day_mapper_.size());
    day_mapper_[day] = id;
    return id;
}

int DataMapper::map_time(const int time)
{
    return time; // minutes from midnight, no conversion needed
}

std::bitset<2> DataMapper::map_week(const std::string& week)
{
    std::bitset<2> result;
    if (week.find('A') != std::string::npos) result.set(0);
    if (week.find('B') != std::string::npos) result.set(1);
    return result;
}

int DataMapper::map_location(const input_models::Location& location)
{
    const auto key = std::make_tuple(location.room, location.building);
    const auto it = location_mapper_.find(key);
    if (it != location_mapper_.end())
    {
        return it->second;
    }

    const int id = static_cast<int>(location_mapper_.size());
    location_mapper_[key] = id;
    return id;
}

int DataMapper::map_lecturer(const std::string& lecturer)
{
    const auto it = lecturer_mapper_.find(lecturer);
    if (it != lecturer_mapper_.end())
    {
        return it->second;
    }

    int id = static_cast<int>(lecturer_mapper_.size());
    lecturer_mapper_[lecturer] = id;
    return id;
}

int DataMapper::map_sequence(const int sequence)
{
    const auto it = sequence_mapper_.find(sequence);
    if (it != sequence_mapper_.end())
    {
        return it->second;
    }

    int id = static_cast<int>(sequence_mapper_.size());
    sequence_mapper_[sequence] = id;
    return id;
}

void DataMapper::remove_sequence(const int sequence)
{
    const auto it = sequence_mapper_.find(sequence);
    if (it != sequence_mapper_.end())
    {
        sequence_mapper_.erase(sequence);
    }
}

// -------------------- DEMAPPERS --------------------

std::tuple<std::string, std::string>
DataMapper::demap_class_id_and_class_type(const int id) const
{
    const auto it = class_id_demapper_.find(id);
    if (it != class_id_demapper_.end())
    {
        return it->second;
    }
    return {"", ""};
}

std::optional<int> DataMapper::demap_group(const int class_id, const int group) const
{
    const auto groups = group_demapper_.find(class_id);
    if (groups != group_demapper_.end())
    {
        const auto it = groups->second.find(group);
        if (it != groups->second.end())
        {
            return it->second;
        }
    }
    return std::nullopt;
}

// -------------------- HELPERS ------------------------

std::optional<int>
DataMapper::find_class_id_and_class_type(const std::string& class_id,
                                         const std::string& class_type) const
{
    const auto key = std::make_tuple(class_id, class_type);
    const auto it = class_id_mapper_.find(key);
    if (it != class_id_mapper_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_group(const int class_id, const int group) const
{
    const auto groups = group_mapper_.find(class_id);
    if (groups != group_mapper_.end())
    {
        const auto it = groups->second.find(group);
        if (it != groups->second.end())
        {
            return it->second;
        }
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_lecturer(const std::string& lecturer) const
{
    const auto it = lecturer_mapper_.find(lecturer);
    if (it != lecturer_mapper_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_day(const int day) const
{
    const auto it = day_mapper_.find(day);
    if (it != day_mapper_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_date(const std::string& date) const
{
    const auto it = date_mapper_.find(date);
    if (it != date_mapper_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_time(const int time) const
{
    return time;
}

// -------------------- MAP CLASSES --------------------

TimeTableProblem DataMapper::map_problem()
{
    auto classes = map_classes();
    auto constraints = map_constraints();
    return TimeTableProblem(std::move(classes), std::move(constraints));
}

std::vector<solver_models::Class> DataMapper::map_classes()
{
    if (!timetable_)
    {
        return {};
    }

    std::vector<solver_models::Class> classes;
    classes.reserve(timetable_->classes.size());

    for (const auto& c : timetable_->classes)
    {
        solver_models::Class sc;
        sc.id        = map_class_id_and_class_type(c.id, c.class_type);
        sc.lecturer  = map_lecturer(c.lecturer);
        sc.day       = map_day(c.day);
        sc.week      = map_week(c.week);
        sc.location  = map_location(c.location);
        sc.group     = map_group(sc.id, c.group);
        sc.start_time = map_time(c.start_time);
        sc.end_time   = map_time(c.end_time);

        for (const auto& s : c.sessions)
        {
            solver_models::Session ss;
            ss.date       = map_date(s.date);
            ss.location   = map_location(s.location);
            ss.start_time = map_time(s.start_time);
            ss.end_time   = map_time(s.end_time);
            sc.sessions.push_back(ss);
        }

        classes.push_back(sc);
    }
    return classes;
}

// -------------------- MAP CONSTRAINTS --------------------

std::vector<solver_models::ConstraintVariant> DataMapper::map_constraints()
{
    if (!timetable_)
    {
        return {};
    }

    std::vector<solver_models::ConstraintVariant> constraints;
    constraints.reserve(timetable_->constraints.size());

    for (const auto& c : timetable_->constraints)
    {
        const int sequence = map_sequence(c.sequence);
        const double weight = c.weight.value_or(1.0);
        const bool hard = c.hard.value_or(false);
        const int slack = c.slack.value_or(0);

        if (c.type == "minimize_gaps")
        {
            constraints.emplace_back(solver_models::MinimizeGapsConstraint{
                sequence, weight, hard, slack,
                c.min_break_duration.value_or(0)
            });
        }
        else if (c.type == "group_preference" && c.class_id && c.class_type && c.group)
        {
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            const std::optional<int> group = find_group(cid.value_or(-1), c.group.value());
            if (cid && group)
            {
                constraints.emplace_back(solver_models::GroupPreferenceConstraint{
                    sequence, weight, hard, slack,
                    cid.value(),
                    group.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!cid)
                {
                    std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
                }
                if (!group && cid)
                {
                    std::cerr << "Could not find mapped group for: " << c.group.value() << " in class: " << cid.value() << std::endl;
                }
            }
        }
        else if (c.type == "lecturer_preference" && c.class_id && c.class_type && c.lecturer)
        {
            const std::optional<int> lecturer_id = find_lecturer(c.lecturer.value());
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            if (cid && lecturer_id)
            {
                constraints.emplace_back(solver_models::LecturerPreferenceConstraint{
                    sequence, weight, hard, slack,
                    cid.value(),
                    lecturer_id.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!cid)
                {
                    std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
                }
                if (!lecturer_id)
                {
                    std::cerr << "Could not find mapped lecturer for: " << c.lecturer.value() << std::endl;
                }
            }
        }
        else if (c.type == "minimize_class_absence" && c.class_id && c.class_type)
        {
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            if (cid)
            {
                constraints.emplace_back(solver_models::MinimizeClassAbsenceConstraint{
                    sequence, weight, hard, slack,
                    cid.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
            }
        }
        else if (c.type == "minimize_group_absence" && c.class_id && c.class_type && c.group)
        {
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            const std::optional<int> group = find_group(cid.value_or(-1), c.group.value());
            if (cid && group)
            {
                constraints.emplace_back(solver_models::MinimizeGroupAbsenceConstraint{
                    sequence, weight, hard, slack,
                    cid.value(),
                    group.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!cid)
                {
                    std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
                }
                if (!group && cid)
                {
                    std::cerr << "Could not find mapped group for: " << c.group.value() << " in class: " << cid.value() << std::endl;
                }
            }
        }
        else if (c.type == "minimize_total_absence")
        {
            constraints.emplace_back(solver_models::MinimizeTotalAbsenceConstraint{
                sequence, weight, hard, slack
            });
        }
        else if (c.type == "time_block_day" && c.start_time && c.end_time && c.day)
        {
            const std::optional<int> start_time = find_time(c.start_time.value());
            const std::optional<int> end_time = find_time(c.end_time.value());
            const std::optional<int> day = find_day(c.day.value());
            if (start_time && end_time && day)
            {
                constraints.emplace_back(solver_models::TimeBlockDayConstraint{
                    sequence, weight, hard, slack,
                    start_time.value(),
                    end_time.value(),
                    day.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!start_time)
                {
                    std::cerr << "Could not find mapped time for: " << c.start_time.value() << std::endl;
                }
                if (!end_time)
                {
                    std::cerr << "Could not find mapped time for: " << c.end_time.value() << std::endl;
                }
                if (!day)
                {
                    std::cerr << "Could not find mapped day for: " << c.day.value() << std::endl;
                }
            }
        }
        else if (c.type == "time_block_date" && c.start_time && c.end_time && c.date)
        {
            const std::optional<int> start_time = find_time(c.start_time.value());
            const std::optional<int> end_time = find_time(c.end_time.value());
            const std::optional<int> date = find_date(c.date.value());
            if (start_time && end_time && date)
            {
            constraints.emplace_back(solver_models::TimeBlockDateConstraint{
                sequence, weight, hard, slack,
                start_time.value(),
                end_time.value(),
                date.value()
            });
            }
            else
            {
                remove_sequence(sequence);
                if (!start_time)
                {
                    std::cerr << "Could not find mapped time for: " << c.start_time.value() << std::endl;
                }
                if (!end_time)
                {
                    std::cerr << "Could not find mapped time for: " << c.end_time.value() << std::endl;
                }
                if (!date)
                {
                    std::cerr << "Could not find mapped date for: " << c.date.value() << std::endl;
                }
            }
        }
        else if (c.type == "prefer_edge_class" && c.class_id && c.class_type && c.position)
        {
            std::optional<solver_models::EdgePosition> pos = std::nullopt;
            if (c.position.value() == "end")
            {
                pos = solver_models::EdgePosition::End;
            }
            else if (c.position.value() == "start")
            {
                pos = solver_models::EdgePosition::Start;
            }
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            if (cid && pos)
            {
                constraints.emplace_back(solver_models::PreferEdgeClassConstraint{
                    sequence, weight, hard, slack,
                    cid.value(),
                    pos.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!cid)
                {
                    std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
                }
                if (!pos)
                {
                    std::cerr << "Could not find mapped position for: " << c.position.value() << std::endl;
                }
            }
        }
        else if (c.type == "prefer_edge_group" && c.class_id && c.class_type && c.position && c.group)
        {
            std::optional<solver_models::EdgePosition> pos = std::nullopt;
            if (c.position.value() == "end")
            {
                pos = solver_models::EdgePosition::End;
            }
            else if (c.position.value() == "start")
            {
                pos = solver_models::EdgePosition::Start;
            }
            const std::optional<int> cid = find_class_id_and_class_type(c.class_id.value(), c.class_type.value());
            const std::optional<int> group = find_group(cid.value_or(-1), c.group.value());
            if (cid && pos && group)
            {
                constraints.emplace_back(solver_models::PreferEdgeGroupConstraint{
                    sequence, weight, hard, slack,
                    cid.value(),
                    group.value(),
                    pos.value()
                });
            }
            else
            {
                remove_sequence(sequence);
                if (!cid)
                {
                    std::cerr << "Could not find mapped class for: " << c.class_id.value() << " with type: " << c.class_type.value() << std::endl;
                }
                if (!pos)
                {
                    std::cerr << "Could not find mapped position for: " << c.position.value() << std::endl;
                }
                if (!group && cid)
                {
                    std::cerr << "Could not find mapped group for: " << c.group.value() << " in class: " << cid.value() << std::endl;
                }
            }
        }
        else
        {
            remove_sequence(sequence);
            std::cerr << "Could not find mapped constraint for: " << c.type << std::endl;
        }
    }

    return constraints;
}

// -------------------- GET PROBLEM --------------------

const TimeTableProblem& DataMapper::get_problem()
{
    if (!problem_ && timetable_)
    {
        problem_ = map_problem();
    }

    return problem_.value();
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

Json DataMapper::get_solution(const std::vector<TimeTableState>& solutions)
{
    solutions_ = solutions;
    return get_solution();
}

Json DataMapper::get_solution() const
{
    if (!problem_)
    {
        return Json{};
    }

    const auto& solutions = solutions_;
    const auto& all_classes = timetable_ ? timetable_->classes : decltype(timetable_->classes){};
    const auto& constraints = timetable_ ? timetable_->constraints : decltype(timetable_->constraints){};

    // Build one result entry per solution
    Json results = Json::array();
    for (const auto& state : solutions)
    {
        // Demap int IDs → string class_id + class_type, then find the full input class.
        Json chosen = Json::array();
        const auto& groups = state.get_assigned_groups();
        for (int class_id = 0; class_id < groups.size(); ++class_id)
        {
            const int group = groups[class_id];
            auto [class_id_str, class_type_str] = demap_class_id_and_class_type(class_id);
            if (class_id_str.empty())
            {
                continue;
            }

            const auto demapped_group = demap_group(class_id, group);

            if (!demapped_group)
            {
                std::cerr << "Unable to find group: " << group << " in class: " << class_id << std::endl;
                continue;
            }

            for (const auto& c : all_classes)
            {
                if (c.group == demapped_group && c.id == class_id_str && c.class_type == class_type_str)
                {
                    chosen.push_back(class_to_json(c));
                    break;
                }
            }
        }

        Json constraint_results = Json::array();
        for (const auto& c : constraints)
            constraint_results.push_back(constraint_result_to_json(c));

        results.push_back(Json{
            {"chosen_classes",     chosen},
            {"constraint_results", constraint_results}
        });
    }

    Json summary = Json{
        {"feasible",                   !solutions.empty()},
        {"hard_constraints_satisfied", false},
        {"score", Json{
            {"total", 0.0},
            {"hard",  0.0},
            {"soft",  0.0}
        }}
    };

    return Json{{"summary", summary}, {"results", results}};
}

// -------------------- TIMETABLE DISPLAY --------------------

void DataMapper::print_timetable(const TimeTableState& state, std::ostream& out) const
{
    // honestly could not be bothered checking this logis, maybe later
    if (!timetable_ || !problem_)
    {
        return;
    }

    const auto& all_classes = timetable_->classes;

    // --- Collect chosen classes ---
    // assigned_only_set: classes with negative group — student is assigned but does not attend
    std::vector<const input_models::Class*> chosen;
    std::set<const input_models::Class*> assigned_only_set;
    for (const int class_id : state.get_classes())
    {
        const int group = state.get_raw_group(class_id);
        if (group == TimeTableState::UNASSIGNED)
        {
            continue;
        }
        const bool assigned_only = group < 0;
        const int effective_group = std::abs(group);
        const auto [cid_str, ctype_str] = demap_class_id_and_class_type(class_id);
        if (cid_str.empty())
        {
            std::cerr << "Skipping: " << class_id << " group: " << group << std::endl;
            continue;
        }
        const auto orig_group = demap_group(class_id, effective_group);
        for (const auto& c : all_classes)
        {
            if (c.group == orig_group && c.id == cid_str && c.class_type == ctype_str)
            {
                chosen.push_back(&c);
                if (assigned_only)
                    assigned_only_set.insert(&c);
                break;
            }
        }
    }

    // --- ANSI 256-color palette (distinct background colors) ---
    static const std::array<int, 16> PALETTE = {
        196, 82, 226, 21, 201, 51, 208, 46, 105, 87, 203, 118, 129, 214, 39, 198
    };
    static const std::string RESET = "\033[0m";

    // Assign a unique ANSI 256-color index to each (class_id, class_type) pair.
    // Assigned-only classes are always gray and do not consume a palette slot.
    std::map<std::pair<std::string, std::string>, int> class_color;
    {
        int idx = 0;
        for (const auto* cls : chosen)
        {
            if (assigned_only_set.contains(cls)) continue;
            auto key = std::make_pair(cls->id, cls->class_type);
            if (!class_color.contains(key))
                class_color[key] = PALETTE[idx++ % static_cast<int>(PALETTE.size())];
        }
    }

    static constexpr int GRAY_FG = 244; // medium gray foreground
    static constexpr int GRAY_BG = 240; // dark gray background

    auto fg_code = [&](const input_models::Class* cls) -> std::string
    {
        if (assigned_only_set.contains(cls))
            return "\033[38;5;" + std::to_string(GRAY_FG) + "m";
        return "\033[38;5;" + std::to_string(class_color.at({cls->id, cls->class_type})) + "m";
    };
    auto bg_code = [&](const input_models::Class* cls) -> std::string
    {
        if (assigned_only_set.contains(cls))
            return "\033[48;5;" + std::to_string(GRAY_BG) + "m";
        return "\033[48;5;" + std::to_string(class_color.at({cls->id, cls->class_type})) + "m";
    };

    static constexpr int SLOT      = 30;
    static constexpr int HALF      = 15;
    static constexpr int DAY_COUNT = 5;
    static constexpr int T_START   = 7  * 60;
    static constexpr int T_END     = 21 * 60 + 30;
    static constexpr int CELL_W    = 6;

    static constexpr std::array<const char*, DAY_COUNT> DAY_NAMES = {
        " pon  ", "wtorek", "środa ", " czw  ", "piątek"
    };

    // n repetitions of ▀ (U+2580) or ▄ (U+2584)
    auto upper_blocks = [](int n) -> std::string
    {
        std::string s; s.reserve(n * 3);
        for (int i = 0; i < n; ++i) s += "\xe2\x96\x80";
        return s;
    };
    auto lower_blocks = [](int n) -> std::string
    {
        std::string s; s.reserve(n * 3);
        for (int i = 0; i < n; ++i) s += "\xe2\x96\x84";
        return s;
    };

    // --- Lane assignment: greedy interval coloring so overlapping classes
    //     are displayed side-by-side, each in its own fixed-width lane. ---
    struct LaneEntry { const input_models::Class* cls; int lane; };
    std::map<std::pair<int, int>, std::vector<LaneEntry>> lane_map;

    for (int d = 0; d < DAY_COUNT; ++d)
    for (int wi = 0; wi < 2; ++wi)
    {
        char week = wi ? 'B' : 'A';
        std::vector<const input_models::Class*> day_cls;
        for (const auto* cls : chosen)
        {
            if ((cls->day - 1) != d) continue;
            if (cls->week.find(week) == std::string::npos) continue;
            day_cls.push_back(cls);
        }
        std::sort(day_cls.begin(), day_cls.end(), [](const auto* a, const auto* b)
        {
            if (a->start_time != b->start_time) return a->start_time < b->start_time;
            return a->id < b->id;
        });

        std::vector<int> lane_ends;
        std::vector<LaneEntry> entries;
        for (const auto* cls : day_cls)
        {
            int lane = -1;
            for (int i = 0; i < static_cast<int>(lane_ends.size()); ++i)
            {
                if (lane_ends[i] <= cls->start_time)
                {
                    lane = i;
                    lane_ends[i] = cls->end_time;
                    break;
                }
            }
            if (lane == -1)
            {
                lane = static_cast<int>(lane_ends.size());
                lane_ends.push_back(cls->end_time);
            }
            entries.push_back({cls, lane});
        }
        lane_map[{d, wi}] = std::move(entries);
    }

    auto n_lanes = [&](int d, int wi) -> int
    {
        int n = 0;
        for (const auto& e : lane_map.at({d, wi}))
            n = std::max(n, e.lane + 1);
        return std::max(n, 1);
    };

    auto get_lane_cls = [&](int T, int d, int wi, int lane) -> const input_models::Class*
    {
        for (const auto& e : lane_map.at({d, wi}))
            if (e.lane == lane && e.cls->start_time <= T && e.cls->end_time > T)
                return e.cls;
        return nullptr;
    };
    auto get_lane_cls_ending_at = [&](int T, int d, int wi, int lane) -> const input_models::Class*
    {
        for (const auto& e : lane_map.at({d, wi}))
            if (e.lane == lane && e.cls->end_time == T)
                return e.cls;
        return nullptr;
    };
    auto get_lane_cls_starting_at = [&](int T, int d, int wi, int lane) -> const input_models::Class*
    {
        for (const auto& e : lane_map.at({d, wi}))
            if (e.lane == lane && e.cls->start_time == T)
                return e.cls;
        return nullptr;
    };

    // Render one lane cell (CELL_W visual columns) for time slot [T, T+SLOT).
    // 15-min transitions use Unicode half-block characters:
    //   ▀ fg=class color  → upper 15 min occupied by that class
    //   ▄ fg=class color  → lower 15 min occupied by that class
    //   When both halves belong to different classes, ▀ is drawn with
    //   the ending class as foreground and the starting class as background.
    auto render_lane = [&](int T, int d, int wi, int lane) -> std::string
    {
        const auto* active      = get_lane_cls(T, d, wi, lane);
        const auto* just_ended  = get_lane_cls_ending_at(T, d, wi, lane);
        const auto* starts_half = get_lane_cls_starting_at(T + HALF, d, wi, lane);

        if (active)
        {
            if (active->end_time == T + HALF)
            {
                if (starts_half)
                    return bg_code(starts_half) + fg_code(active) + upper_blocks(CELL_W) + RESET;
                return fg_code(active) + upper_blocks(CELL_W) + RESET;
            }
            return bg_code(active) + std::string(CELL_W, ' ') + RESET;
        }

        if (just_ended && starts_half)
            return bg_code(starts_half) + fg_code(just_ended) + upper_blocks(CELL_W) + RESET;
        if (just_ended)
            return fg_code(just_ended) + upper_blocks(CELL_W) + RESET;
        if (starts_half)
            return fg_code(starts_half) + lower_blocks(CELL_W) + RESET;

        return std::string(CELL_W, ' '); // NOLINT
    };

    auto render_day = [&](int T, int d, int wi) -> std::string
    {
        std::string s;
        for (int l = 0; l < n_lanes(d, wi); ++l)
            s += render_lane(T, d, wi, l);
        return s;
    };

    // Pad day name (always 6 visual columns) to column visual width.
    auto pad_day = [](const char* name, int col_w) -> std::string
    {
        std::string s(name);
        int extra = col_w - 6; // all DAY_NAMES are exactly 6 visual columns
        if (extra > 0) s.append(extra, ' ');
        return s;
    };

    // --- Header ---
    out << "    A";
    for (int d = 0; d < DAY_COUNT; ++d)
        out << "|" << pad_day(DAY_NAMES[d], n_lanes(d, 0) * CELL_W);
    out << "|    B";
    for (int d = 0; d < DAY_COUNT; ++d)
        out << "|" << pad_day(DAY_NAMES[d], n_lanes(d, 1) * CELL_W);
    out << "|\n";

    // --- Time rows ---
    for (int T = T_START; T < T_END; T += SLOT)
    {
        char buf[6];
        std::snprintf(buf, sizeof(buf), "%2d:%02d", T / 60, T % 60);

        out << buf;
        for (int d = 0; d < DAY_COUNT; ++d)
            out << "|" << render_day(T, d, 0);
        out << "|";

        out << buf;
        for (int d = 0; d < DAY_COUNT; ++d)
            out << "|" << render_day(T, d, 1);
        out << "|\n";
    }

    // --- Legend ---
    out << "\n";
    for (const auto& [key, color_val] : class_color)
    {
        const auto& [cid, ctype] = key;
        std::string lecturer;
        bool is_assigned_only = false;
        for (const auto* cls : chosen)
        {
            if (cls->id == cid && cls->class_type == ctype)
            {
                lecturer = cls->lecturer;
                is_assigned_only = assigned_only_set.contains(cls);
                break;
            }
        }
        if (is_assigned_only)
        {
            std::cout << "FUCK" << std::endl;
            out << "\033[48;5;" << GRAY_BG << "m" << std::string(CELL_W, ' ') << RESET
                << "  " << cid << "  [" << ctype << "]  " << lecturer << "  (assigned only)\n";
        }
        else
        {
            out << "\033[48;5;" << color_val << "m" << std::string(CELL_W, ' ') << RESET
                << "  " << cid << "  [" << ctype << "]  " << lecturer << "\n";
        }
    }
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const DataMapper& m)
{
    const int n_classes = m.timetable_ ? static_cast<int>(m.timetable_->classes.size())     : 0;
    const int n_constraints = m.timetable_ ? static_cast<int>(m.timetable_->constraints.size()) : 0;
    const int n_solutions = static_cast<int>(m.solutions_.size());

    out << "InputDataMapper{ classes=" << n_classes
        << ", constraints=" << n_constraints
        << ", solutions=" << n_solutions
        << " }";
    return out;
}

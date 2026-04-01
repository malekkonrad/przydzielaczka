//
// Created by mateu on 30.03.2026.
//

#include <data_mapper.h>
#include <tuple_string_hash.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <ostream>
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
    c.min_break_duration = get_opt<int>(j, "min_break_duration");
    c.preferred_group = get_opt<int>(j, "preferred_group");
    c.preferred_lecturer = get_opt<std::string>(j, "preferred_lecturer");
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
    if (c.slack) j["slack"] = *c.slack;
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

bool DataMapper::validate(const Json& data)
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
    return day; // already 0-indexed in the input
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

std::optional<int> DataMapper::find_lecturer(const std::string& lecturer) const
{
    const auto it = lecturer_mapper_.find(lecturer);
    if (it != lecturer_mapper_.end())
    {
        return it->second;
    }
    return std::nullopt;
}

std::optional<int> DataMapper::find_day(int day) const
{
    return day;
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

// -------------------- MAP CLASSES --------------------

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
        sc.group     = c.group;
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
        const double weight = c.weight.value_or(1.0);
        const bool hard = c.hard.value_or(false);
        const int slack = c.slack.value_or(0);

        if (c.type == "minimize_gaps")
        {
            constraints.push_back(solver_models::MinimizeGapsConstraint{
                c.sequence, weight, hard, slack,
                c.min_break_duration.value_or(0)
            });
        }
        else if (c.type == "group_preference" && c.class_id && c.class_type && c.preferred_group)
        {
            const std::optional<int> cid = find_class_id_and_class_type(*c.class_id, *c.class_type);
            if (cid)
            {
                constraints.push_back(solver_models::GroupPreferenceConstraint{
                    c.sequence, weight, hard, slack,
                    cid.value(),
                    c.preferred_group.value()
                });
            } else
            {
                std::cerr << "Could not find mapped class for: " << *c.class_id << " with type: " << *c.class_type << std::endl;
            }
        }
        else if (c.type == "lecturer_preference" && c.class_id && c.class_type && c.preferred_lecturer)
        {
            const std::optional<int> lecturer_id = find_lecturer(*c.preferred_lecturer);
            const std::optional<int> cid = find_class_id_and_class_type(*c.class_id, *c.class_type);
            if (cid && lecturer_id)
            {
                constraints.push_back(solver_models::LecturerPreferenceConstraint{
                    c.sequence, weight, hard, slack,
                    cid.value(),
                    lecturer_id.value()
                });
            }
        }
        else if (c.type == "maximize_single_attendance" && c.class_id && c.class_type)
        {
            const std::optional<int> cid = find_class_id_and_class_type(*c.class_id, *c.class_type);
            if (cid)
            {
                constraints.push_back(solver_models::MaximizeSingleAttendanceConstraint{
                    c.sequence, weight, hard, slack,
                    cid.value()
                });
            }
        }
        else if (c.type == "maximize_total_attendance")
        {
            constraints.push_back(solver_models::MaximizeTotalAttendanceConstraint{
                c.sequence, weight, hard, slack
            });
        }
        else if (c.type == "time_block_day" && c.start_time && c.end_time && c.day)
        {
            constraints.push_back(solver_models::TimeBlockDayConstraint{
                c.sequence, weight, hard, slack,
                map_time(c.start_time.value()),
                map_time(c.end_time.value()),
                map_day(c.day.value())
            });
        }
        else if (c.type == "time_block_date" && c.date)
        {
            constraints.push_back(solver_models::TimeBlockDateConstraint{
                c.sequence, weight, hard, slack,
                map_time(c.start_time.value_or(0)),
                map_time(c.end_time.value_or(0)),
                map_date(*c.date)
            });
        }
        else if (c.type == "prefer_edge_classes" && c.class_id)
        {
            solver_models::EdgePosition pos = solver_models::EdgePosition::Start;
            if (c.position && *c.position == "end")
                pos = solver_models::EdgePosition::End;

            for (int cid : find_class_int_ids(*c.class_id))
            {
                constraints.push_back(solver_models::PreferEdgeClassesConstraint{
                    c.sequence, weight, hard, slack, cid, pos
                });
            }
        }
    }

    return constraints;
}

// -------------------- GET PROBLEM --------------------

const TimeTableProblem& DataMapper::get_problem()
{
    if (!problem_.has_value() && timetable_.has_value())
        problem_ = TimeTableProblem(map_classes(), map_constraints());

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
        return Json{};

    const auto& solutions = solutions_;
    const auto& all_classes = timetable_ ? timetable_->classes : decltype(timetable_->classes){};
    const auto& constraints = timetable_ ? timetable_->constraints : decltype(timetable_->constraints){};

    // Build one result entry per solution
    Json results = Json::array();
    for (const auto& state : solutions)
    {
        // Demap int IDs → string class_id + class_type, then find the full input class.
        Json chosen = Json::array();
        for (const int id : state.get_chosen_ids())
        {
            auto [class_id_str, class_type_str] = demap_class_id_and_class_type(id);
            if (class_id_str.empty())
                continue;

            for (const auto& c : all_classes)
            {
                if (c.id == class_id_str && c.class_type == class_type_str)
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

// UTF-8 block characters used for the timetable cells.
static const std::string CELL_FULL  = "\xe2\x96\x88"; // █  U+2588
static const std::string CELL_LOWER = "\xe2\x96\x84"; // ▄  U+2584  (class starts next slot)
static const std::string CELL_UPPER = "\xe2\x96\x80"; // ▀  U+2580  (class just ended)
static const std::string CELL_EMPTY = "      ";        // 6 spaces

static std::string make_class_cell(char abbr)
{
    // Pattern: ███X██  (6 visual columns, 16 bytes due to 3-byte UTF-8 block chars)
    return CELL_FULL + CELL_FULL + CELL_FULL + abbr + CELL_FULL + CELL_FULL;
}

static std::string make_lower_row() { return CELL_LOWER + CELL_LOWER + CELL_LOWER
                                           + CELL_LOWER + CELL_LOWER + CELL_LOWER; }
static std::string make_upper_row() { return CELL_UPPER + CELL_UPPER + CELL_UPPER
                                           + CELL_UPPER + CELL_UPPER + CELL_UPPER; }

void DataMapper::print_timetable(const TimeTableState& state, std::ostream& out) const
{
    if (!timetable_) return;

    const auto& all_classes = timetable_->classes;

    // Demap int IDs → string class_id + class_type, then find the full input class.
    std::vector<const input_models::Class*> chosen;
    for (const int id : state.get_chosen_ids())
    {
        auto [class_id_str, class_type_str] = demap_class_id_and_class_type(id);
        if (class_id_str.empty())
            continue;
        for (const auto& c : all_classes)
        {
            if (c.id == class_id_str && c.class_type == class_type_str)
            {
                chosen.push_back(&c);
                break;
            }
        }
    }

    static constexpr int SLOT      = 30;   // minutes per row
    static constexpr int DAY_COUNT = 5;    // Mon–Fri
    static constexpr int T_START   = 7  * 60;          // 7:00
    static constexpr int T_END     = 21 * 60 + 30;     // 21:30

    // Polish day names, exactly 6 visible columns each.
    static constexpr std::array<const char*, DAY_COUNT> DAY_NAMES = {
        " pon  ", "wtorek", "\xc5\x9broda ", " czw  ", "pi\xc4\x85tek"
        //         środa (ś = c5 9b)               piątek (ą = c4 85)
    };

    // Returns the first chosen class active at minute T on the given day and week letter.
    auto find_active = [&](int T, int day, char week) -> const input_models::Class*
    {
        for (const auto* cls : chosen)
        {
            if (cls->day != day) continue;
            if (cls->week.find(week) == std::string::npos) continue;
            if (cls->start_time <= T && cls->end_time > T)
                return cls;
        }
        return nullptr;
    };

    // True if any chosen class on this day/week starts strictly within (T, T+SLOT].
    auto next_starts = [&](int T, int day, char week) -> bool
    {
        for (const auto* cls : chosen)
        {
            if (cls->day != day) continue;
            if (cls->week.find(week) == std::string::npos) continue;
            if (cls->start_time > T && cls->start_time <= T + SLOT)
                return true;
        }
        return false;
    };

    // Render one 6-column cell for time slot [T, T+SLOT).
    auto render_cell = [&](int T, int day, char week) -> std::string
    {
        if (const auto* cls = find_active(T, day, week))
        {
            const char abbr = cls->class_type.empty()
                ? '?'
                : static_cast<char>(std::toupper(static_cast<unsigned char>(cls->class_type[0])));
            return make_class_cell(abbr);
        }

        if (next_starts(T, day, week))
            return make_lower_row();   // ▄▄▄▄▄▄  class starts next slot

        // Class ended at T (was active one slot earlier)?
        if (T >= SLOT && find_active(T - SLOT, day, week))
            return make_upper_row();   // ▀▀▀▀▀▀  class just ended

        return CELL_EMPTY;
    };

    // ---- Header ----
    out << "    A";
    for (int d = 0; d < DAY_COUNT; ++d) out << "|" << DAY_NAMES[d];
    out << "|    B";
    for (int d = 0; d < DAY_COUNT; ++d) out << "|" << DAY_NAMES[d];
    out << "|\n";

    // ---- Time rows ----
    for (int T = T_START; T < T_END; T += SLOT)
    {
        char time_buf[6];
        std::snprintf(time_buf, sizeof(time_buf), "%2d:%02d", T / 60, T % 60);

        // Week A
        out << time_buf;
        for (int d = 0; d < DAY_COUNT; ++d)
            out << "|" << render_cell(T, d, 'A');
        out << "|";

        // Week B (same time label, right half)
        out << time_buf;
        for (int d = 0; d < DAY_COUNT; ++d)
            out << "|" << render_cell(T, d, 'B');
        out << "|\n";
    }
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const DataMapper& m)
{
    int n_classes     = m.timetable_ ? static_cast<int>(m.timetable_->classes.size())     : 0;
    int n_constraints = m.timetable_ ? static_cast<int>(m.timetable_->constraints.size()) : 0;
    int n_solutions   = static_cast<int>(m.solutions_.size());

    out << "InputDataMapper{ classes=" << n_classes
        << ", constraints=" << n_constraints
        << ", solutions=" << n_solutions
        << " }";
    return out;
}

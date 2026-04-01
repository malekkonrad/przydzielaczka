//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <bitset>
#include <variant>

namespace input_models
{
    struct Location
    {
        std::string room;
        std::string building;
    };

    struct Session
    {
        std::string date;
        Location location;
        int start_time;
        int end_time;
    };

    struct Class
    {
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

    struct Constraint
    {
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
        std::optional<int> slack;
    };

    struct Timetable
    {
        std::vector<Constraint> constraints;
        std::vector<Class> classes;
    };

} // namespace input_models

namespace solver_models
{
    struct Session
    {
        int date;
        int location;
        int start_time;
        int end_time;
    };

    struct Class
    {
        int id;
        int lecturer;
        int day;
        std::bitset<2> week; // (00, 01, 10 11) - none, even, odd, both
        int location;
        int group;
        int start_time;
        int end_time;
        std::vector<Session> sessions;
    };

    enum class ConstraintType
    {
        MinimizeGaps,
        GroupPreference,
        LecturerPreference,
        MaximizeSingleAttendance,
        MaximizeTotalAttendance,
        TimeBlockDay,
        TimeBlockDate,
        PreferEdgeClasses
    };

    enum class EdgePosition { Start, End };

    struct MinimizeGapsConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int min_break;
    };

    struct GroupPreferenceConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int class_id;
        int preferred_group;
    };

    struct LecturerPreferenceConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int class_id;
        int lecturer;
    };

    struct TimeBlockDayConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int start_time;
        int end_time;
        int day;
    };

    struct TimeBlockDateConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int start_time;
        int end_time;
        int date;
    };

    struct PreferEdgeClassesConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int class_id;
        EdgePosition position;
    };

    struct MaximizeSingleAttendanceConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        int class_id;
    };

    struct MaximizeTotalAttendanceConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
    };

    using ConstraintVariant = std::variant<
        MinimizeGapsConstraint,
        GroupPreferenceConstraint,
        LecturerPreferenceConstraint,
        TimeBlockDayConstraint,
        TimeBlockDateConstraint,
        PreferEdgeClassesConstraint,
        MaximizeSingleAttendanceConstraint,
        MaximizeTotalAttendanceConstraint
    >;

} // namespace solver_models
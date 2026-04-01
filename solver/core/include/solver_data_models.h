//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <bitset>
#include <variant>
#include <vector>

namespace solver_models
{
    using TimeTableTime = int;
    using TimeTableDate = int;
    using TimeTableDay = int;
    using TimeTableWeek = std::bitset<2>;

    struct Session
    {
        TimeTableDate date;
        int location;
        TimeTableTime start_time;
        TimeTableTime end_time;
    };

    struct Class
    {
        int id;
        int lecturer;
        TimeTableDay day;
        TimeTableWeek week;
        int location;
        int group;
        TimeTableTime start_time;
        TimeTableTime end_time;
        std::vector<Session> sessions; // TODO remove the vector
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
        TimeTableTime start_time;
        TimeTableTime end_time;
        TimeTableDay day;
    };

    struct TimeBlockDateConstraint
    {
        int sequence;
        double weight;
        bool hard;
        int slack;
        TimeTableTime start_time;
        TimeTableTime end_time;
        TimeTableDate date;
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
//
// Created by mateu on 31.03.2026.
//

#pragma once

#include <string>
#include <vector>
#include <optional>

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
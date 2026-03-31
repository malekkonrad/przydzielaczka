//
// Created by mateu on 30.03.2026.
//

#include "constraints.h"

#include <variant>

#include <nlohmann/json.hpp>

// ConstraintVariant createConstraint(const nlohmann::json& j) {
//     std::string type = j.at("constraint_type");
//
//     int seq = j.at("sequence");
//     double weight = j.value("weight", 1.0);
//     bool hard = j.value("hard", false);
//
//     if (type == "minimize_gaps") {
//         return MinimizeGapsConstraint{
//             seq, weight, hard,
//             j.at("min_break_duration")
//         };
//     }
//
//     if (type == "group_preference") {
//         return GroupPreferenceConstraint{
//             seq, weight, hard,
//             j.at("class_id"),
//             j.at("preferred_group")
//         };
//     }
//
//     if (type == "lecturer_preference") {
//         return LecturerPreferenceConstraint{
//             seq, weight, hard,
//             j.at("class_id"),
//             j.at("preferred_lecturer")
//         };
//     }
//
//     if (type == "time_block") {
//         return TimeBlockConstraint{
//             seq, weight, hard,
//             j.at("start_time"),
//             j.at("end_time"),
//             j.contains("day") ? std::optional<int>(j["day"]) : std::nullopt,
//             j.contains("date") ? std::optional<std::string>(j["date"]) : std::nullopt
//         };
//     }
//
//     if (type == "prefer_edge_classes") {
//         EdgePosition pos = j.at("position") == "start"
//             ? EdgePosition::Start
//             : EdgePosition::End;
//
//         return PreferEdgeClassesConstraint{
//             seq, weight, hard,
//             j.at("class_id"),
//             pos
//         };
//     }
//
//     if (type == "maximize_single_attendance") {
//         return MaximizeSingleAttendanceConstraint{
//             seq, weight, hard,
//             j.at("class_id")
//         };
//     }
//
//     if (type == "maximize_total_attendance") {
//         return MaximizeTotalAttendanceConstraint{
//             seq, weight, hard
//         };
//     }
//
//     throw std::runtime_error("Unknown constraint type");
// }
//
// struct TimetableState {
//     // your data representation
// };
//
// double evaluateConstraint(const ConstraintVariant& c, const TimetableState& state) {
//     return std::visit([&](const auto& constraint) -> double {
//
//         using T = std::decay_t<decltype(constraint)>;
//
//         if constexpr (std::is_same_v<T, MinimizeGapsConstraint>) {
//             // TODO: implement
//             return 0.0;
//         }
//
//         else if constexpr (std::is_same_v<T, GroupPreferenceConstraint>) {
//             // TODO
//             return 0.0;
//         }
//
//         else if constexpr (std::is_same_v<T, TimeBlockConstraint>) {
//             // TODO
//             return 0.0;
//         }
//
//         else {
//             return 0.0;
//         }
//
//     }, c);
// }
//
// double evaluateSolution(
//     const std::vector<ConstraintVariant>& constraints,
//     const TimetableState& state
// ) {
//     double score = 0.0;
//
//     for (const auto& c : constraints) {
//         double penalty = evaluateConstraint(c, state);
//
//         std::visit([&](const auto& constraint) {
//             if (constraint.hard && penalty > 0.0) {
//                 score += 1e9; // or reject solution
//             } else {
//                 score += constraint.weight * penalty;
//             }
//         }, c);
//     }
//
//     return score;
// }
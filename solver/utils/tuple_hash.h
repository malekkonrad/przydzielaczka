//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <string>
#include <tuple>

// Hash for unordered_map keyed on tuple<string,string>
template<>
struct std::hash<std::tuple<std::string, std::string>>
{
    size_t operator()(const std::tuple<std::string, std::string>& p) const noexcept
    {
        size_t h1 = std::hash<std::string>{}(std::get<0>(p));
        size_t h2 = std::hash<std::string>{}(std::get<1>(p));
        return h1 ^ (h2 * 2654435761ULL); // Knuth multiplicative hash mix
    }
};

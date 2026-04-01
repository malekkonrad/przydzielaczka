//
// Created by mateu on 01.04.2026.
//

#pragma once

#include <string>
#include <tuple>

// Hash for unordered_map keyed on pair<string,string>
template<>
struct std::hash<std::tuple<std::string, std::string>>
{
    std::size_t operator()(const std::tuple<std::string, std::string>& p) const noexcept
    {
        std::size_t h1 = std::hash<std::string>{}(std::get<0>(p));
        std::size_t h2 = std::hash<std::string>{}(std::get<1>(p));
        return h1 ^ (h2 * 2654435761ULL); // Knuth multiplicative hash mix
    }
}; // namespace std
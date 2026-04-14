//
// Created by mateu on 06.04.2026.
//

#pragma once

#include <limits>
#include <vector>

// Holds the best penalty score per constraint (indexed by constraint id)
// observed after solving a sequence. Passed to is_feasible() when solving
// subsequent sequences to enforce slack-bounded feasibility.
// Empty best_scores means no previous sequence has been solved yet.
class SequenceContext
{
public:
    static constexpr double EMPTY = std::numeric_limits<double>::infinity();

    explicit SequenceContext(size_t size);

    [[nodiscard]] bool   has_score(int id) const;
    void                 set_score(int id, double score);
    [[nodiscard]] double get_score(int id) const;

    double& operator[](int id);
    double  operator[](int id) const;

    [[nodiscard]] size_t size() const;

    // Sum of all finite (non-EMPTY) scores — useful for display.
    [[nodiscard]] double sum() const;

    // Lexicographic comparison on the scores vector.
    [[nodiscard]] bool operator< (const SequenceContext& other) const;
    [[nodiscard]] bool operator> (const SequenceContext& other) const;
    [[nodiscard]] bool operator<=(const SequenceContext& other) const;
    [[nodiscard]] bool operator>=(const SequenceContext& other) const;
    [[nodiscard]] bool operator==(const SequenceContext& other) const;
    [[nodiscard]] bool operator!=(const SequenceContext& other) const;

    friend std::ostream& operator<<(std::ostream& out, const SequenceContext& context);

private:
    std::vector<double> scores_;
};

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

    explicit SequenceContext(size_t constraint_size, size_t sequence_size);
    explicit SequenceContext(size_t constraint_size, size_t sequence_size, int value);

    [[nodiscard]] bool   has_sequence_score(int id) const;
    void                 set_sequence_score(int id, double score);
    [[nodiscard]] double get_sequence_score(int id) const;

    [[nodiscard]] bool   has_constraint_score(int id) const;
    void                 set_constraint_score(int id, double score);
    [[nodiscard]] double get_constraint_score(int id) const;

    [[nodiscard]] size_t sequence_size() const;
    [[nodiscard]] size_t constraint_size() const;

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
    std::vector<double> sequence_scores_;
    std::vector<double> constraint_scores_;
};

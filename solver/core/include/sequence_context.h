//
// Created by mateu on 06.04.2026.
//

#pragma once


// Holds the best penalty score per constraint (indexed by constraint id)
// observed after solving a sequence. Passed to is_feasible() when solving
// subsequent sequences to enforce slack-bounded feasibility.
// Empty best_scores means no previous sequence has been solved yet.
// TODO add comparator etc
// TODO maybe do a const size vector in utils
class SequenceContext
{
public:
    static constexpr double EMPTY = std::numeric_limits<double>::infinity();

    explicit SequenceContext(const size_t size)
        : scores_(size, EMPTY) {}

    [[nodiscard]] bool has_score(const int id) const
    {
        return scores_[id] != EMPTY;
    }

    void set_score(const int id, const double score)
    {
        scores_[id] = score;
    }

    [[nodiscard]] double get_score(const int id) const
    {
        return scores_[id];
    }

    double& operator[](const int id)
    {
        return scores_[id];
    }

    [[nodiscard]] size_t size() const
    {
        return scores_.size();
    }
private:
    std::vector<double> scores_;
};
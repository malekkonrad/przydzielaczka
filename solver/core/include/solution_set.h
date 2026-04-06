//
// Created by mateu on 06.04.2026.
//

#pragma once

#include <algorithm>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>
#include <time_table_state.h>

// Ranked — any type that supports strict weak ordering via operator<.
template<typename Score>
concept Ranked = requires(const Score& a, const Score& b) {
    { a < b } -> std::convertible_to<bool>;
};

// BoundedSolutionSet<Score> — keeps the N best (lowest-score) entries seen so far.
//
// Internally a max-heap over a std::vector (worst = front), so:
//   worst_score()  O(1)
//   best_score()   O(1)  — tracked via a separately maintained minimum
//   insert()       O(log N)
//   no per-node allocation, contiguous storage
//
// Call extract() to consume the set and retrieve (score, state) pairs sorted
// best-first. The set is left valid-but-empty afterwards.
template<Ranked Score>
class BoundedSolutionSet
{
    using Entry = std::pair<Score, TimeTableState>;

    // max-heap: entry with larger Score is "greater" — worst bubbles to front.
    static bool heap_cmp(const Entry& a, const Entry& b) { return a.first < b.first; }

public:
    explicit BoundedSolutionSet(const std::size_t max_n)
        : max_n_(max_n)
    {
        assert(max_n > 0);
        heap_.reserve(max_n);
    }

    void clear()
    {
        heap_.clear();
        best_.reset();
    }

    [[nodiscard]] std::size_t size()  const noexcept { return heap_.size(); }
    [[nodiscard]] bool        empty() const noexcept { return heap_.empty(); }

    // Only valid when !empty().
    [[nodiscard]] const Score& best_score()  const noexcept { return *best_; }
    [[nodiscard]] const Score& worst_score() const noexcept { return heap_.front().first; }

    void insert(Score score, TimeTableState state)
    {
        if (heap_.size() < max_n_)
        {
            if (!best_ || score < *best_) best_ = score;
            heap_.push_back({std::move(score), std::move(state)});
            std::push_heap(heap_.begin(), heap_.end(), heap_cmp);
        }
        else if (score < heap_.front().first)
        {
            if (!best_ || score < *best_) best_ = score;
            std::pop_heap(heap_.begin(), heap_.end(), heap_cmp);
            heap_.back() = {std::move(score), std::move(state)};
            std::push_heap(heap_.begin(), heap_.end(), heap_cmp);
        }
    }

    // Consumes the set — returns (score, state) pairs sorted best-first.
    // The set is left valid and empty.
    [[nodiscard]] std::vector<Entry> extract()
    {
        std::sort(heap_.begin(), heap_.end(), heap_cmp);
        std::vector<Entry> result = std::move(heap_);
        heap_.clear();
        best_.reset();
        return result;
    }

    // Convenience overload — discards scores, returns states only.
    [[nodiscard]] std::vector<TimeTableState> extract_states()
    {
        std::sort(heap_.begin(), heap_.end(), heap_cmp);
        std::vector<TimeTableState> result;
        result.reserve(heap_.size());
        for (auto& [s, st] : heap_)
            result.push_back(std::move(st));
        heap_.clear();
        best_.reset();
        return result;
    }

private:
    std::size_t        max_n_;
    std::vector<Entry> heap_;
    std::optional<Score> best_;
};

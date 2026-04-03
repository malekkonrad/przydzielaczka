//
// Created by mateu on 30.03.2026.
//

#include "time_table_problem.h"

#include <algorithm>
#include <ostream>
#include <variant>

// -------------------- HELPERS --------------------

static int constraint_sequence(const solver_models::ConstraintVariant& v)
{
    return std::visit([](const auto& c) { return c.sequence; }, v);
}

static bool constraint_is_hard(const solver_models::ConstraintVariant& v)
{
    return std::visit([](const auto& c) { return c.hard; }, v);
}

// -------------------- CONSTRUCTORS --------------------

TimeTableProblem::TimeTableProblem(
    std::vector<solver_models::Class> classes,
    std::vector<solver_models::ConstraintVariant> constraints)
    : classes_(std::move(classes))
    , constraints_(std::move(constraints))
{
    // Sort: ascending sequence, hard before soft within the same sequence.
    std::stable_sort(constraints_.begin(), constraints_.end(),
        [](const solver_models::ConstraintVariant& a, const solver_models::ConstraintVariant& b)
        {
            const int sa = constraint_sequence(a);
            const int sb = constraint_sequence(b);
            if (sa != sb)
            {
                return sa < sb;
            }
            return constraint_is_hard(a) && !constraint_is_hard(b);
        });

    // Build sequence_split_point_: for each sequence index, store the position of
    // its first soft constraint (or constraints_.size() if it has none).
    // This lets get_goals() jump directly to the soft section in O(1).
    if (!constraints_.empty())
    {
        const int max_seq = constraint_sequence(constraints_.back());
        sequence_split_point_.assign(max_seq + 1, static_cast<int>(constraints_.size()));

        for (int i = 0; i < static_cast<int>(constraints_.size()); ++i)
        {
            const int seq = constraint_sequence(constraints_[i]);
            if (!constraint_is_hard(constraints_[i]) &&
                sequence_split_point_[seq] == static_cast<int>(constraints_.size()))
            {
                sequence_split_point_[seq] = i;
            }
        }
    }
}

TimeTableProblem::TimeTableProblem(TimeTableProblem&& other) noexcept
    : classes_(std::move(other.classes_))
    , constraints_(std::move(other.constraints_))
    , sequence_split_point_(std::move(other.sequence_split_point_))
{
}

TimeTableProblem& TimeTableProblem::operator=(TimeTableProblem&& other) noexcept
{
    if (this != &other)
    {
        classes_               = std::move(other.classes_);
        constraints_           = std::move(other.constraints_);
        sequence_split_point_  = std::move(other.sequence_split_point_);
    }
    return *this;
}

// -------------------- ACCESSORS --------------------

size_t TimeTableProblem::size() const
{
    return classes_.size();
}

size_t TimeTableProblem::sequence_size() const
{
    return sequence_split_point_.size();
}

const std::vector<solver_models::Class>& TimeTableProblem::get_classes() const
{
    return classes_;
}

const std::vector<solver_models::ConstraintVariant>& TimeTableProblem::get_constraints() const
{
    return constraints_;
}

// Returns all hard+soft constraints for sequences < sequence,
// plus hard-only constraints for exactly sequence.
const std::vector<solver_models::ConstraintVariant>& TimeTableProblem::get_constraints(const int sequence) const
{
    // Thread-local cache so we can return a stable reference.
    thread_local std::vector<solver_models::ConstraintVariant> result;
    result.clear();

    for (const auto& c : constraints_)
    {
        const int seq = constraint_sequence(c);
        if (seq < sequence)
        {
            result.push_back(c);
        }
        else if (seq == sequence && constraint_is_hard(c))
        {
            result.push_back(c);
        }
        else
        {
            break; // sorted: everything remaining is soft-at-sequence or beyond
        }
    }
    return result;
}

// Returns the soft constraints whose sequence equals exactly sequence.
// Uses sequence_split_point_ to skip directly to the soft section.
const std::vector<solver_models::ConstraintVariant>& TimeTableProblem::get_goals(const int sequence) const
{
    thread_local std::vector<solver_models::ConstraintVariant> result;
    result.clear();

    if (sequence >= static_cast<int>(sequence_split_point_.size()))
        return result;

    const int start = sequence_split_point_[sequence];
    if (start >= static_cast<int>(constraints_.size()))
        return result;

    for (int i = start; i < static_cast<int>(constraints_.size()); ++i)
    {
        const auto& c = constraints_[i];
        if (constraint_sequence(c) != sequence) break;
        result.push_back(c);
    }
    return result;
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p)
{
    out << "TimeTableProblem{ classes=" << p.classes_.size()
        << ", constraints=" << p.constraints_.size()
        << ", sequences=" << p.sequence_split_point_.size() << " }";
    return out;
}
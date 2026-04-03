//
// Created by mateu on 30.03.2026.
//

#include "time_table_problem.h"

#include <algorithm>
#include <ostream>
#include <span>
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
    const int max_class_id = std::max_element(classes_.begin(), classes_.end(),
        [](const auto& a, const auto& b){return a.id < b.id;})->id;
    max_group_.assign(max_class_id + 1, 0);
    for (const auto& c : classes_)
    {
        const auto id = static_cast<size_t>(c.id);
        max_group_[id] = std::max(c.group, max_group_[id]);
    }

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

    // Build sequence_soft_hard_split_point_: for each sequence index, store the position of
    // its first soft constraint or index of the end of the sequence.
    // This lets get_goals() jump directly to the soft section in O(1).
    if (!constraints_.empty())
    {
        const int max_seq = constraint_sequence(constraints_.back());
        sequence_soft_hard_split_point_.assign(max_seq + 1, static_cast<int>(constraints_.size()));
        sequence_split_point_.resize(max_seq + 1);

        for (int i = 0; i < static_cast<int>(constraints_.size()); i++)
        {
            const auto& constraint = constraints_[i];
            const int seq = constraint_sequence(constraint);
            const bool is_hard = constraint_is_hard(constraint);
            sequence_split_point_[seq] = i;
            if (!is_hard && sequence_soft_hard_split_point_[seq] == static_cast<int>(constraints_.size()))
            {
                sequence_soft_hard_split_point_[seq] = i;
            }
        }
        // fill the soft_hard with the end of the sequence
        for (int i = 0; i < sequence_split_point_.size(); i++)
        {
            if (sequence_soft_hard_split_point_[i] == static_cast<int>(constraints_.size()))
            {
                sequence_soft_hard_split_point_[i] = sequence_split_point_[i];
            }
        }
    }
}

TimeTableProblem::TimeTableProblem(TimeTableProblem&& other) noexcept
    : max_group_(std::move(other.max_group_))
    , classes_(std::move(other.classes_))
    , constraints_(std::move(other.constraints_))
    , sequence_soft_hard_split_point_(std::move(other.sequence_soft_hard_split_point_))
    , sequence_split_point_(std::move(other.sequence_split_point_))
{
}

TimeTableProblem& TimeTableProblem::operator=(TimeTableProblem&& other) noexcept
{
    if (this != &other)
    {
        max_group_             = std::move(other.max_group_);
        classes_               = std::move(other.classes_);
        constraints_           = std::move(other.constraints_);
        sequence_soft_hard_split_point_ = std::move(other.sequence_soft_hard_split_point_);
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

size_t TimeTableProblem::class_size() const
{
    return max_group_.size();
}

const std::vector<int>& TimeTableProblem::get_max_group() const
{
    return max_group_;
}

int TimeTableProblem::get_max_group(const int class_id) const
{
    return max_group_[static_cast<size_t>(class_id)];
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
// Equivalent to constraints_[0 .. sequence_split_point_[sequence]).
std::span<const solver_models::ConstraintVariant> TimeTableProblem::get_constraints(const int sequence) const
{
    if (sequence >= static_cast<int>(sequence_split_point_.size()))
    {
        return constraints_;
    }

    const auto end = static_cast<size_t>(sequence_split_point_[sequence]);
    return std::span(constraints_).first(end);
}

// Returns the soft constraints whose sequence equals exactly sequence.
// Uses sequence_split_point_ to jump directly to the soft section.
std::span<const solver_models::ConstraintVariant> TimeTableProblem::get_goals(const int sequence) const
{
    if (sequence >= static_cast<int>(sequence_split_point_.size()))
    {
        return {};
    }

    const auto start = static_cast<size_t>(sequence_soft_hard_split_point_[sequence]);
    const auto end = static_cast<size_t>(sequence_split_point_[sequence]);

    return std::span(constraints_).subspan(start, end - start);
}

// -------------------- STREAM --------------------

std::ostream& operator<<(std::ostream& out, const TimeTableProblem& p)
{
    out << "TimeTableProblem{ classes=" << p.classes_.size()
        << ", constraints=" << p.constraints_.size()
        << ", sequences=" << p.sequence_split_point_.size() << " }";
    return out;
}
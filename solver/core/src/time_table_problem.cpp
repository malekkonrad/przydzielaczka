//
// Created by mateu on 30.03.2026.
//

#include "time_table_problem.h"
#include <constraints.h>

#include <algorithm>
#include <ostream>
#include <span>
#include <variant>
#include <ranges>

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
    : constraints_(std::move(constraints))
    , max_date_(0)
    , max_day_(0)
{
    const int max_class_id = std::max_element(classes.begin(), classes.end(),
        [](const auto& a, const auto& b){return a.id < b.id;})->id;
    max_group_.assign(max_class_id + 1, 0);
    for (const auto& c : classes)
    {
        const auto id = static_cast<size_t>(c.id);
        max_group_[id] = std::max(c.group, max_group_[id]);
    }

    // Create classes map [class_id][group] -> Class
    classes_.resize(max_group_.size());
    for (int i = 0; i < max_group_.size(); i++)
    {
        classes_[i].resize(max_group_[i]);
    }
    for (const auto& clazz : classes)
    {
        classes_[clazz.id][clazz.group - 1] = clazz;
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

    // Assign stable ids — index in the sorted constraints vector.
    for (int i = 0; i < static_cast<int>(constraints_.size()); ++i)
    {
        std::visit([i](auto& c){ c.id = i; }, constraints_[i]);
    }

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
            sequence_split_point_[seq] = i + 1;
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

    // Get the max date
    for (const auto& c : classes)
    {
        for (const auto& s: c.sessions)
        {
            max_date_ = std::max(max_date_, s.date);
        }
    }

    // Get the max day
    for (const auto& c : classes)
    {
        max_day_ = std::max(max_day_, c.day);
    }
}

TimeTableProblem::TimeTableProblem(TimeTableProblem&& other) noexcept
    : max_group_(std::move(other.max_group_))
    , classes_(std::move(other.classes_))
    , constraints_(std::move(other.constraints_))
    , sequence_soft_hard_split_point_(std::move(other.sequence_soft_hard_split_point_))
    , sequence_split_point_(std::move(other.sequence_split_point_))
    , max_date_(std::move(other.max_date_))
    , max_day_(std::move(other.max_day_))
    , weeks_(std::move(other.weeks_))
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
        max_date_              = std::move(other.max_date_);
        max_day_               = std::move(other.max_day_);
        weeks_                 = std::move(other.weeks_);
    }
    return *this;
}

// -------------------- ACCESSORS --------------------

size_t TimeTableProblem::size() const
{
    return std::transform_reduce(classes_.begin(), classes_.end(), 0ULL, std::plus{},
        [](const auto& v) { return v.size(); });
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
    return max_group_[class_id];
}

int TimeTableProblem::get_max_date() const
{
    return max_date_;
}

int TimeTableProblem::get_max_day() const
{
    return max_day_;
}

int TimeTableProblem::get_weeks() const
{
    return weeks_;
}

const std::vector<std::vector<solver_models::Class>>& TimeTableProblem::get_classes() const
{
    return classes_;
}

const std::vector<solver_models::Class>& TimeTableProblem::get_groups(const int class_id) const
{
    return classes_[class_id];
}

const solver_models::Class& TimeTableProblem::get_group(const int class_id, const int group) const
{
    return classes_[class_id][group - 1];
}

const std::vector<solver_models::ConstraintVariant>& TimeTableProblem::get_constraints() const
{
    return constraints_;
}

std::span<const solver_models::ConstraintVariant> TimeTableProblem::constraints_in(const int sequence) const
{
    if (sequence >= static_cast<int>(sequence_split_point_.size()))
    {
        return constraints_;
    }

    const size_t start = sequence < 1 ? 0 : static_cast<size_t>(sequence_split_point_[sequence - 1]);
    const auto end = static_cast<size_t>(sequence_split_point_[sequence]);

    return std::span(constraints_).subspan(start, end - start);
}

std::span<const solver_models::ConstraintVariant> TimeTableProblem::constraints_up_to(const int sequence) const
{
    return constraints_before(sequence + 1);
}

// Returns all hard+soft constraints for sequences < sequence,
// plus hard-only constraints for exactly sequence.
// Equivalent to constraints_[0 .. sequence_split_point_[sequence]).
std::span<const solver_models::ConstraintVariant> TimeTableProblem::constraints_before(const int sequence) const
{
    if (sequence >= static_cast<int>(sequence_split_point_.size()))
    {
        return constraints_;
    }

    const auto end = sequence < 1 ? 0 : static_cast<size_t>(sequence_split_point_[sequence - 1]);
    return std::span(constraints_).first(end);
}

std::span<const solver_models::ConstraintVariant> TimeTableProblem::hard_constraints_in(const int sequence) const
{
    if (sequence >= static_cast<int>(sequence_split_point_.size()))
    {
        return constraints_;
    }

    const size_t start = sequence < 1 ? 0 : static_cast<size_t>(sequence_split_point_[sequence - 1]);
    const auto end = static_cast<size_t>(sequence_soft_hard_split_point_[sequence]);

    return std::span(constraints_).subspan(start, end - start);
}

// Returns the soft constraints whose sequence equals exactly sequence.
// Uses sequence_split_point_ to jump directly to the soft section.
std::span<const solver_models::ConstraintVariant> TimeTableProblem::goals_in(const int sequence) const
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
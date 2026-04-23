//
// Created by mateu on 06.04.2026.
//

#include <sequence_context.h>

SequenceContext::SequenceContext(const size_t constraint_size, const size_t sequence_size)
    : sequence_scores_(sequence_size, EMPTY), constraint_scores_(constraint_size, EMPTY) {}

SequenceContext::SequenceContext(const size_t constraint_size, const size_t sequence_size, int value)
    : sequence_scores_(sequence_size, value), constraint_scores_(constraint_size, value) {}

bool SequenceContext::has_sequence_score(const int id) const { return sequence_scores_[id] != EMPTY; }
void SequenceContext::set_sequence_score(const int id, const double score) { sequence_scores_[id] = score; }
double SequenceContext::get_sequence_score(const int id) const { return sequence_scores_[id]; }

bool SequenceContext::has_constraint_score(const int id) const { return constraint_scores_[id] != EMPTY; }
void SequenceContext::set_constraint_score(const int id, const double score) { constraint_scores_[id] = score; }
double SequenceContext::get_constraint_score(const int id) const { return constraint_scores_[id]; }

size_t SequenceContext::sequence_size() const { return sequence_scores_.size(); }
size_t SequenceContext::constraint_size() const { return constraint_scores_.size(); }

double SequenceContext::sum() const
{
    double total = 0.0;
    for (const double s : sequence_scores_)
        if (s != EMPTY) total += s;
    return total;
}

bool SequenceContext::operator< (const SequenceContext& other) const { return sequence_scores_ <  other.sequence_scores_; }
bool SequenceContext::operator> (const SequenceContext& other) const { return sequence_scores_ >  other.sequence_scores_; }
bool SequenceContext::operator<=(const SequenceContext& other) const { return sequence_scores_ <= other.sequence_scores_; }
bool SequenceContext::operator>=(const SequenceContext& other) const { return sequence_scores_ >= other.sequence_scores_; }
bool SequenceContext::operator==(const SequenceContext& other) const { return sequence_scores_ == other.sequence_scores_; }
bool SequenceContext::operator!=(const SequenceContext& other) const { return sequence_scores_ != other.sequence_scores_; }

std::ostream& operator<<(std::ostream& out, const SequenceContext& context)
{
    out << "SequenceContext{ ";
    if (!context.sequence_scores_.empty())
    {
        for (int i = 0; i < context.sequence_size() - 1; ++i)
        {
            out << context.get_sequence_score(i) << ", ";
        }
        out << context.get_sequence_score(context.sequence_size() - 1);
    }
    if (!context.constraint_scores_.empty())
    {
        for (int i = 0; i < context.constraint_size() - 1; ++i)
        {
            out << context.get_constraint_score(i) << ", ";
        }
        out << context.get_sequence_score(context.constraint_size() - 1);
    }
    out << " }";
    return out;
}

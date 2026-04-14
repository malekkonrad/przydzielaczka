//
// Created by mateu on 06.04.2026.
//

#include <sequence_context.h>

SequenceContext::SequenceContext(const size_t size)
    : scores_(size, EMPTY) {}

bool SequenceContext::has_score(const int id) const { return scores_[id] != EMPTY; }
void SequenceContext::set_score(const int id, const double score) { scores_[id] = score; }
double SequenceContext::get_score(const int id) const { return scores_[id]; }

double& SequenceContext::operator[](const int id)       { return scores_[id]; }
double  SequenceContext::operator[](const int id) const { return scores_[id]; }

size_t SequenceContext::size() const { return scores_.size(); }

double SequenceContext::sum() const
{
    double total = 0.0;
    for (const double s : scores_)
        if (s != EMPTY) total += s;
    return total;
}

bool SequenceContext::operator< (const SequenceContext& o) const { return scores_ <  o.scores_; }
bool SequenceContext::operator> (const SequenceContext& o) const { return scores_ >  o.scores_; }
bool SequenceContext::operator<=(const SequenceContext& o) const { return scores_ <= o.scores_; }
bool SequenceContext::operator>=(const SequenceContext& o) const { return scores_ >= o.scores_; }
bool SequenceContext::operator==(const SequenceContext& o) const { return scores_ == o.scores_; }
bool SequenceContext::operator!=(const SequenceContext& o) const { return scores_ != o.scores_; }

std::ostream& operator<<(std::ostream& out, const SequenceContext& context)
{
    out << "SequenceContext{ ";
    if (context.scores_.size() > 0)
    {
        for (int i = 0; i < context.size() - 1; ++i)
        {
            out << context[i] << ", ";
        }
        out << context[context.size() - 1];
        out << " }";
    }
    return out;
}

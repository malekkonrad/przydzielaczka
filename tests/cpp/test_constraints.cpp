#include <gtest/gtest.h>
#include "constraints.h"
#include "helpers.h"

using namespace solver_models;
using namespace test;

// Convenience wrapper so individual tests don't repeat the long signature.
static double eval_one(const ConstraintVariant& c,
                       const TimeTableState& state,
                       const TimeTableProblem& problem)
{
    return constraints::evaluate(c, state, problem);
}

// ============================================================
//  MinimizeGapsConstraint
// ============================================================

class MinimizeGapsTest : public ::testing::Test
{
protected:
    // class 1: day 0, 08:00–09:00
    // class 2: day 0, 10:00–11:00   → gap between them = 100 units
    Class cls1 = make_class(1, /*day*/0, both_weeks(), 800, 900);
    Class cls2 = make_class(2, /*day*/0, both_weeks(), 1000, 1100);

    ConstraintVariant no_break_allowed = MinimizeGapsConstraint{0, 1.0, false, /*min_break*/0};
};

TEST_F(MinimizeGapsTest, EmptyStateYieldsZeroPenalty)
{
    auto problem = make_problem({cls1, cls2});
    EXPECT_DOUBLE_EQ(eval_one(no_break_allowed, make_state({}), problem), 0.0);
}

TEST_F(MinimizeGapsTest, SingleClassOnDayYieldsZeroPenalty)
{
    auto problem = make_problem({cls1, cls2});
    EXPECT_DOUBLE_EQ(eval_one(no_break_allowed, make_state({1}), problem), 0.0);
}

TEST_F(MinimizeGapsTest, GapExceedsMinBreakProducesPenalty)
{
    auto problem = make_problem({cls1, cls2});
    // gap = 100, min_break = 0  →  penalty = 100
    EXPECT_GT(eval_one(no_break_allowed, make_state({1, 2}), problem), 0.0);
}

TEST_F(MinimizeGapsTest, GapWithinMinBreakYieldsZeroPenalty)
{
    ConstraintVariant large_break = MinimizeGapsConstraint{0, 1.0, false, /*min_break*/200};
    auto problem = make_problem({cls1, cls2});
    // gap = 100 ≤ min_break 200  →  no penalty
    EXPECT_DOUBLE_EQ(eval_one(large_break, make_state({1, 2}), problem), 0.0);
}

TEST_F(MinimizeGapsTest, ClassesOnDifferentDaysProduceNoGapPenalty)
{
    Class cls3 = make_class(3, /*day*/1, both_weeks(), 1000, 1100);
    auto problem = make_problem({cls1, cls3});
    EXPECT_DOUBLE_EQ(eval_one(no_break_allowed, make_state({1, 3}), problem), 0.0);
}

TEST_F(MinimizeGapsTest, PenaltyEqualsExcessGap)
{
    auto problem = make_problem({cls1, cls2});
    // gap = 100 (1000 - 900), min_break = 40  →  excess = 60
    ConstraintVariant c = MinimizeGapsConstraint{0, 1.0, false, /*min_break*/40};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), problem), 60.0);
}

// ============================================================
//  GroupPreferenceConstraint
// ============================================================

TEST(GroupPreference, ClassNotInStateYieldsZeroPenalty)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, /*group*/5);
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, 1, /*preferred*/5};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), make_problem({cls})), 0.0);
}

TEST(GroupPreference, PreferredGroupYieldsZeroPenalty)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, /*group*/5);
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, 1, /*preferred*/5};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 0.0);
}

TEST(GroupPreference, WrongGroupYieldsPenaltyOne)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, /*group*/3);
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, 1, /*preferred*/5};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 1.0);
}

// ============================================================
//  LecturerPreferenceConstraint
// ============================================================

TEST(LecturerPreference, ClassNotInStateYieldsZeroPenalty)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, 0, /*lecturer*/7);
    ConstraintVariant c = LecturerPreferenceConstraint{0, 1.0, false, 1, /*preferred*/7};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), make_problem({cls})), 0.0);
}

TEST(LecturerPreference, PreferredLecturerYieldsZeroPenalty)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, 0, /*lecturer*/7);
    ConstraintVariant c = LecturerPreferenceConstraint{0, 1.0, false, 1, /*preferred*/7};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 0.0);
}

TEST(LecturerPreference, WrongLecturerYieldsPenaltyOne)
{
    Class cls = make_class(1, 0, both_weeks(), 800, 900, 0, /*lecturer*/3);
    ConstraintVariant c = LecturerPreferenceConstraint{0, 1.0, false, 1, /*preferred*/7};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 1.0);
}

// ============================================================
//  MinimizeClassAbsenceConstraint
// ============================================================

TEST(MinimizeClassAbsence, ClassChosenYieldsZeroPenalty)
{
    ConstraintVariant c = MinimizeClassAbsenceConstraint{0, 1.0, false, 1};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({make_class(1)})), 0.0);
}

TEST(MinimizeClassAbsence, ClassAbsentYieldsPenaltyOne)
{
    ConstraintVariant c = MinimizeClassAbsenceConstraint{0, 1.0, false, 1};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), make_problem({make_class(1)})), 1.0);
}

// ============================================================
//  MinimizeTotalAbsenceConstraint
// ============================================================

TEST(MinimizeTotalAbsence, AllClassesChosenYieldsZeroPenalty)
{
    ConstraintVariant c = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    auto problem = make_problem({make_class(1), make_class(2)});
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), problem), 0.0);
}

TEST(MinimizeTotalAbsence, NoneChosenYieldsPenaltyOne)
{
    ConstraintVariant c = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    auto problem = make_problem({make_class(1), make_class(2)});
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), problem), 1.0);
}

TEST(MinimizeTotalAbsence, HalfChosenYieldsPenaltyHalf)
{
    ConstraintVariant c = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    auto problem = make_problem({make_class(1), make_class(2)});
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), problem), 0.5);
}

TEST(MinimizeTotalAbsence, EmptyProblemYieldsZeroPenalty)
{
    ConstraintVariant c = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), make_problem({})), 0.0);
}

// ============================================================
//  TimeBlockDayConstraint
// ============================================================

TEST(TimeBlockDay, NoClassOnBlockedDayYieldsZeroPenalty)
{
    // Block day 0, but class is on day 1.
    ConstraintVariant c = TimeBlockDayConstraint{0, 1.0, false, 800, 1000, /*day*/0};
    Class cls = make_class(1, /*day*/1, both_weeks(), 800, 1000);
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 0.0);
}

TEST(TimeBlockDay, ClassOverlapsBlockedTimeYieldsPenalty)
{
    // Block: 09:00–11:00, class: 08:00–10:00 on same day  →  overlap exists
    ConstraintVariant c = TimeBlockDayConstraint{0, 1.0, false, 900, 1100, /*day*/0};
    Class cls = make_class(1, /*day*/0, both_weeks(), 800, 1000);
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 1.0);
}

TEST(TimeBlockDay, ClassAdjacentToBlockButNotOverlappingYieldsZeroPenalty)
{
    // Block starts at 1000, class ends at 1000 — touching but not overlapping.
    ConstraintVariant c = TimeBlockDayConstraint{0, 1.0, false, 1000, 1200, /*day*/0};
    Class cls = make_class(1, /*day*/0, both_weeks(), 800, 1000);
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 0.0);
}

TEST(TimeBlockDay, TwoClassesOnBlockedDayYieldsTwoPenalties)
{
    ConstraintVariant c = TimeBlockDayConstraint{0, 1.0, false, 800, 1200, /*day*/0};
    Class cls1 = make_class(1, /*day*/0, both_weeks(), 800,  900);
    Class cls2 = make_class(2, /*day*/0, both_weeks(), 1000, 1100);
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), make_problem({cls1, cls2})), 2.0);
}

// ============================================================
//  TimeBlockDateConstraint
// ============================================================

TEST(TimeBlockDate, SessionOverlapsBlockedDateYieldsPenalty)
{
    // Block date 20260401, 09:00–11:00
    ConstraintVariant c = TimeBlockDateConstraint{0, 1.0, false, 900, 1100, /*date*/20260401};

    Class cls = make_class(1);
    cls.sessions = {Session{.date = 20260401, .location = 0, .start_time = 800, .end_time = 1000}};

    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 1.0);
}

TEST(TimeBlockDate, SessionOnDifferentDateYieldsZeroPenalty)
{
    ConstraintVariant c = TimeBlockDateConstraint{0, 1.0, false, 900, 1100, /*date*/20260401};

    Class cls = make_class(1);
    cls.sessions = {Session{.date = 20260402, .location = 0, .start_time = 800, .end_time = 1000}};

    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 0.0);
}

TEST(TimeBlockDate, MultipleSessionsOnSameDateCountedOnce)
{
    // Two sessions on the blocked date — the class should count as 1 violation, not 2.
    ConstraintVariant c = TimeBlockDateConstraint{0, 1.0, false, 900, 1100, /*date*/20260401};

    Class cls = make_class(1);
    cls.sessions = {
        Session{.date = 20260401, .location = 0, .start_time = 800, .end_time = 1000},
        Session{.date = 20260401, .location = 0, .start_time = 1000, .end_time = 1100}
    };

    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1}), make_problem({cls})), 1.0);
}

// ============================================================
//  PreferEdgeClassesConstraint
// ============================================================

TEST(PreferEdgeClasses, ClassNotInStateYieldsZeroPenalty)
{
    ConstraintVariant c = PreferEdgeClassConstraint{0, 1.0, false, 1, EdgePosition::Start};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({}), make_problem({make_class(1)})), 0.0);
}

TEST(PreferEdgeClasses, ClassIsEarliestStartOnDayYieldsZeroPenalty)
{
    Class cls1 = make_class(1, 0, both_weeks(), 800,  900);
    Class cls2 = make_class(2, 0, both_weeks(), 1000, 1100);
    // class 1 has the earliest start — satisfies EdgePosition::Start
    ConstraintVariant c = PreferEdgeClassConstraint{0, 1.0, false, 1, EdgePosition::Start};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), make_problem({cls1, cls2})), 0.0);
}

TEST(PreferEdgeClasses, ClassIsNotEarliestStartOnDayYieldsPenaltyOne)
{
    Class cls1 = make_class(1, 0, both_weeks(), 800,  900);
    Class cls2 = make_class(2, 0, both_weeks(), 1000, 1100);
    // class 2 is NOT the earliest — violates EdgePosition::Start
    ConstraintVariant c = PreferEdgeClassConstraint{0, 1.0, false, 2, EdgePosition::Start};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), make_problem({cls1, cls2})), 1.0);
}

TEST(PreferEdgeClasses, ClassIsLatestEndOnDayYieldsZeroPenalty)
{
    Class cls1 = make_class(1, 0, both_weeks(), 800,  900);
    Class cls2 = make_class(2, 0, both_weeks(), 1000, 1100);
    // class 2 has the latest end — satisfies EdgePosition::End
    ConstraintVariant c = PreferEdgeClassConstraint{0, 1.0, false, 2, EdgePosition::End};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), make_problem({cls1, cls2})), 0.0);
}

TEST(PreferEdgeClasses, ClassIsNotLatestEndOnDayYieldsPenaltyOne)
{
    Class cls1 = make_class(1, 0, both_weeks(), 800,  900);
    Class cls2 = make_class(2, 0, both_weeks(), 1000, 1100);
    // class 1 is NOT the latest end
    ConstraintVariant c = PreferEdgeClassConstraint{0, 1.0, false, 1, EdgePosition::End};
    EXPECT_DOUBLE_EQ(eval_one(c, make_state({1, 2}), make_problem({cls1, cls2})), 1.0);
}

// ============================================================
//  evaluate_all  — weighting, hard constraints, aggregation
// ============================================================

TEST(EvaluateAll, EmptyProblemYieldsZeroScore)
{
    EXPECT_DOUBLE_EQ(constraints::evaluate_all(make_problem(), make_state()), 0.0);
}

TEST(EvaluateAll, SoftConstraintWeightIsApplied)
{
    // Penalty = 1.0, weight = 3.5  →  total = 3.5
    ConstraintVariant c = MinimizeClassAbsenceConstraint{0, /*weight*/3.5, /*hard*/false, 1};
    auto problem = make_problem({make_class(1)}, {c});
    EXPECT_DOUBLE_EQ(constraints::evaluate_all(problem, make_state({})), 3.5);
}

TEST(EvaluateAll, HardConstraintViolatedAddsLargePenalty)
{
    ConstraintVariant c = MinimizeClassAbsenceConstraint{0, 1.0, /*hard*/true, 1};
    auto problem = make_problem({make_class(1)}, {c});
    EXPECT_GE(constraints::evaluate_all(problem, make_state({})), 1e9);
}

TEST(EvaluateAll, HardConstraintSatisfiedContributesZero)
{
    ConstraintVariant c = MinimizeClassAbsenceConstraint{0, 1.0, /*hard*/true, 1};
    auto problem = make_problem({make_class(1)}, {c});
    EXPECT_DOUBLE_EQ(constraints::evaluate_all(problem, make_state({1})), 0.0);
}

TEST(EvaluateAll, MultipleSoftConstraintsPenaltiesSummed)
{
    // Both classes absent  →  2.0 + 3.0 = 5.0
    ConstraintVariant c1 = MinimizeClassAbsenceConstraint{0, /*weight*/2.0, false, 1};
    ConstraintVariant c2 = MinimizeClassAbsenceConstraint{1, /*weight*/3.0, false, 2};
    auto problem = make_problem({make_class(1), make_class(2)}, {c1, c2});
    EXPECT_DOUBLE_EQ(constraints::evaluate_all(problem, make_state({})), 5.0);
}

TEST(EvaluateAll, MixOfHardAndSoftConstraints)
{
    // Hard constraint violated → adds >= 1e9.  Soft penalty on top is irrelevant
    // for acceptance but the total must be >= 1e9.
    ConstraintVariant hard = MinimizeClassAbsenceConstraint{0, 1.0, /*hard*/true, 1};
    ConstraintVariant soft = MinimizeClassAbsenceConstraint{1, 5.0, /*hard*/false, 2};
    auto problem = make_problem({make_class(1), make_class(2)}, {hard, soft});
    // Neither class chosen: hard violated + soft penalty 5
    EXPECT_GE(constraints::evaluate_all(problem, make_state({})), 1e9);
}

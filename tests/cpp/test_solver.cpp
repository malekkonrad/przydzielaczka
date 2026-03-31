#include <gtest/gtest.h>
#include "solver.h"
#include "constraints.h"
#include "helpers.h"

#include <sstream>

using namespace solver_models;
using namespace test;

// ============================================================
//  Trivial / empty problems
// ============================================================

TEST(Solver, EmptyProblemYieldsOneEmptySolution)
{
    auto problem = make_problem({}, {});
    auto solutions = Solver(problem).solve();

    ASSERT_EQ(solutions.size(), 1u);
    EXPECT_TRUE(solutions[0].is_empty());
}

TEST(Solver, SingleClassYieldsTwoSolutions)
{
    // Can either include or exclude the one class.
    auto problem = make_problem({make_class(1)}, {});
    auto solutions = Solver(problem).solve(10);
    EXPECT_EQ(solutions.size(), 2u);
}

TEST(Solver, TwoNonConflictingClassesYieldFourSolutions)
{
    // Independent classes on different days → 2 × 2 = 4 combinations.
    auto problem = make_problem({
        make_class(1, /*day*/0),
        make_class(2, /*day*/1)
    }, {});
    auto solutions = Solver(problem).solve(10);
    EXPECT_EQ(solutions.size(), 4u);
}

// ============================================================
//  Time-conflict detection
// ============================================================

TEST(Solver, OverlappingClassesNeverBothChosen)
{
    // Same day, same week, overlapping times — must not coexist.
    Class cls1 = make_class(1, 0, both_weeks(), 800, 1000);
    Class cls2 = make_class(2, 0, both_weeks(), 900, 1100); // overlaps cls1
    auto problem = make_problem({cls1, cls2}, {});

    auto solutions = Solver(problem).solve(100);

    for (const auto& sol : solutions)
        EXPECT_FALSE(sol.contains(1) && sol.contains(2))
            << "Solution must not contain both time-conflicting classes";
}

TEST(Solver, AdjacentClassesNotConsideredConflicting)
{
    // cls1 ends at 900, cls2 starts at 900 — touching but not overlapping.
    Class cls1 = make_class(1, 0, both_weeks(), 800, 900);
    Class cls2 = make_class(2, 0, both_weeks(), 900, 1000);
    auto problem = make_problem({cls1, cls2}, {});

    auto solutions = Solver(problem).solve(10);

    bool found_both = false;
    for (const auto& sol : solutions)
        if (sol.contains(1) && sol.contains(2)) { found_both = true; break; }

    EXPECT_TRUE(found_both) << "Adjacent classes should be combinable";
}

TEST(Solver, ClassesOnDifferentDaysCanBothBeChosen)
{
    Class cls1 = make_class(1, /*day*/0, both_weeks(), 800, 1000);
    Class cls2 = make_class(2, /*day*/1, both_weeks(), 800, 1000);
    auto problem = make_problem({cls1, cls2}, {});

    auto solutions = Solver(problem).solve(10);

    bool found_both = false;
    for (const auto& sol : solutions)
        if (sol.contains(1) && sol.contains(2)) { found_both = true; break; }

    EXPECT_TRUE(found_both);
}

TEST(Solver, ClassesInDifferentWeeksCanBothBeChosen)
{
    // Same day and same timeslot, but different weeks — not a conflict.
    Class cls1 = make_class(1, 0, week_a(), 800, 1000);
    Class cls2 = make_class(2, 0, week_b(), 800, 1000);
    auto problem = make_problem({cls1, cls2}, {});

    auto solutions = Solver(problem).solve(10);

    bool found_both = false;
    for (const auto& sol : solutions)
        if (sol.contains(1) && sol.contains(2)) { found_both = true; break; }

    EXPECT_TRUE(found_both) << "Classes in separate weeks should not conflict";
}

// ============================================================
//  max_solutions limit
// ============================================================

TEST(Solver, MaxSolutionsLimitsOutputCount)
{
    // 3 independent classes → up to 8 solutions, but we cap at 3.
    auto problem = make_problem({
        make_class(1, 0),
        make_class(2, 1),
        make_class(3, 2)
    }, {});

    auto solutions = Solver(problem).solve(/*max_solutions*/3);
    EXPECT_LE(static_cast<int>(solutions.size()), 3);
}

TEST(Solver, MaxSolutionsZeroReturnsNoSolutions)
{
    auto problem = make_problem({make_class(1)}, {});
    auto solutions = Solver(problem).solve(0);
    EXPECT_TRUE(solutions.empty());
}

// ============================================================
//  Sorting by penalty score
// ============================================================

TEST(Solver, SolutionsAreSortedByScoreAscending)
{
    // MaximizeTotalAttendance penalizes absent classes — solutions with more
    // chosen classes will score better (lower penalty).
    ConstraintVariant c = MaximizeTotalAttendanceConstraint{0, 1.0, false};
    auto problem = make_problem({
        make_class(1, 0),
        make_class(2, 1)
    }, {c});

    auto solutions = Solver(problem).solve(10);
    ASSERT_GE(solutions.size(), 2u);

    for (std::size_t i = 1; i < solutions.size(); ++i)
    {
        double prev = constraints::evaluate_all(problem, solutions[i - 1]);
        double curr = constraints::evaluate_all(problem, solutions[i]);
        EXPECT_LE(prev, curr)
            << "Solutions out of order at index " << i
            << " (scores: " << prev << " > " << curr << ")";
    }
}

// ============================================================
//  Hard-constraint filtering
// ============================================================

TEST(Solver, HardConstraintViolationExcludesSolution)
{
    // Hard: class 1 must be attended.
    // The empty solution (class 1 absent) violates the constraint → excluded.
    ConstraintVariant c = MaximizeSingleAttendanceConstraint{0, 1.0, /*hard*/true, 1};
    auto problem = make_problem({make_class(1)}, {c});

    auto solutions = Solver(problem).solve(10);

    for (const auto& sol : solutions)
        EXPECT_TRUE(sol.contains(1))
            << "Hard constraint should filter out solutions that omit class 1";
}

TEST(Solver, HardConstraintSatisfiedSolutionsAreKept)
{
    ConstraintVariant c = MaximizeSingleAttendanceConstraint{0, 1.0, /*hard*/true, 1};
    auto problem = make_problem({make_class(1)}, {c});

    auto solutions = Solver(problem).solve(10);

    // There must be at least one valid solution (the one that includes class 1).
    EXPECT_FALSE(solutions.empty());
}

TEST(Solver, ImpossibleHardConstraintYieldsNoSolutions)
{
    // Two classes that conflict in time, but a hard constraint requires BOTH.
    Class cls1 = make_class(1, 0, both_weeks(), 800, 1000);
    Class cls2 = make_class(2, 0, both_weeks(), 900, 1100);

    ConstraintVariant must_attend_1 = MaximizeSingleAttendanceConstraint{0, 1.0, true, 1};
    ConstraintVariant must_attend_2 = MaximizeSingleAttendanceConstraint{1, 1.0, true, 2};
    auto problem = make_problem({cls1, cls2}, {must_attend_1, must_attend_2});

    auto solutions = Solver(problem).solve(10);
    EXPECT_TRUE(solutions.empty());
}

// ============================================================
//  Stream output
// ============================================================

TEST(Solver, StreamOutputDoesNotCrash)
{
    auto problem = make_problem({make_class(1)}, {});
    Solver solver(problem);
    std::ostringstream oss;
    EXPECT_NO_THROW(oss << solver);
    EXPECT_FALSE(oss.str().empty());
}

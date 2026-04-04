#include <gtest/gtest.h>
#include "time_table_problem.h"
#include "helpers.h"

#include <sstream>
#include <variant>

using namespace solver_models;
using namespace test;

// ============================================================
//  Construction and basic accessors
// ============================================================

TEST(TimeTableProblem, DefaultConstructedHasNoClassesOrConstraints)
{
    TimeTableProblem p;
    EXPECT_TRUE(p.get_classes().empty());
    EXPECT_TRUE(p.get_constraints().empty());
}

TEST(TimeTableProblem, StoresProvidedClasses)
{
    auto classes = std::vector{make_class(1), make_class(2)};
    TimeTableProblem p(classes, {});
    EXPECT_EQ(p.get_classes().size(), 2u);
}

TEST(TimeTableProblem, StoresProvidedConstraints)
{
    ConstraintVariant c = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    TimeTableProblem p({}, {c});
    EXPECT_EQ(p.get_constraints().size(), 1u);
}

// ============================================================
//  find_class
// ============================================================

TEST(TimeTableProblem, FindClassReturnsCorrectClass)
{
    TimeTableProblem p({make_class(42)}, {});
    const Class* cls = p.find_class(42);
    ASSERT_NE(cls, nullptr);
    EXPECT_EQ(cls->id, 42);
}

TEST(TimeTableProblem, FindClassReturnsNullptrForUnknownId)
{
    TimeTableProblem p({make_class(1)}, {});
    EXPECT_EQ(p.find_class(999), nullptr);
}

TEST(TimeTableProblem, FindClassOnEmptyProblemReturnsNullptr)
{
    TimeTableProblem p;
    EXPECT_EQ(p.find_class(0), nullptr);
}

TEST(TimeTableProblem, FindClassDistinguishesMultipleClasses)
{
    TimeTableProblem p({make_class(10), make_class(20), make_class(30)}, {});
    EXPECT_EQ(p.find_class(10)->id, 10);
    EXPECT_EQ(p.find_class(20)->id, 20);
    EXPECT_EQ(p.find_class(30)->id, 30);
}

// ============================================================
//  get_constraints_for_class
// ============================================================

TEST(TimeTableProblem, GetConstraintsForClassFiltersCorrectly)
{
    ConstraintVariant c_for_10  = GroupPreferenceConstraint{0, 1.0, false, 10, 0};
    ConstraintVariant c_for_20  = GroupPreferenceConstraint{1, 1.0, false, 20, 0};
    ConstraintVariant c_global  = MinimizeTotalAbsenceConstraint{2, 1.0, false};

    TimeTableProblem p({make_class(10), make_class(20)}, {c_for_10, c_for_20, c_global});

    auto for_10 = p.get_constraints_for_class(10);
    ASSERT_EQ(for_10.size(), 1u);
    EXPECT_EQ(std::get<GroupPreferenceConstraint>(*for_10[0]).class_id, 10);

    auto for_20 = p.get_constraints_for_class(20);
    ASSERT_EQ(for_20.size(), 1u);
    EXPECT_EQ(std::get<GroupPreferenceConstraint>(*for_20[0]).class_id, 20);
}

TEST(TimeTableProblem, GlobalConstraintsNotReturnedByGetConstraintsForClass)
{
    ConstraintVariant global = MinimizeTotalAbsenceConstraint{0, 1.0, false};
    TimeTableProblem p({make_class(1)}, {global});
    EXPECT_TRUE(p.get_constraints_for_class(1).empty());
}

TEST(TimeTableProblem, GetConstraintsForClassReturnsEmptyForUnknownClassId)
{
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, 5, 0};
    TimeTableProblem p({make_class(5)}, {c});
    EXPECT_TRUE(p.get_constraints_for_class(999).empty());
}

TEST(TimeTableProblem, MultipleConstraintsForSameClassAllReturned)
{
    ConstraintVariant c1 = GroupPreferenceConstraint{0, 1.0, false, 1, 0};
    ConstraintVariant c2 = LecturerPreferenceConstraint{1, 1.0, false, 1, 0};
    ConstraintVariant c3 = MinimizeClassAbsenceConstraint{2, 1.0, false, 1};

    TimeTableProblem p({make_class(1)}, {c1, c2, c3});
    EXPECT_EQ(p.get_constraints_for_class(1).size(), 3u);
}

// ============================================================
//  is_valid
// ============================================================

TEST(TimeTableProblem, EmptyProblemIsValid)
{
    EXPECT_TRUE(TimeTableProblem().is_valid());
}

TEST(TimeTableProblem, ValidWhenAllClassTimesAreCorrect)
{
    TimeTableProblem p({make_class(1, 0, both_weeks(), 800, 1000)}, {});
    EXPECT_TRUE(p.is_valid());
}

TEST(TimeTableProblem, InvalidWhenStartTimeEqualsEndTime)
{
    TimeTableProblem p({make_class(1, 0, both_weeks(), 800, 800)}, {});
    EXPECT_FALSE(p.is_valid());
}

TEST(TimeTableProblem, InvalidWhenStartTimeAfterEndTime)
{
    TimeTableProblem p({make_class(1, 0, both_weeks(), 1000, 800)}, {});
    EXPECT_FALSE(p.is_valid());
}

TEST(TimeTableProblem, InvalidWhenConstraintReferencesNonExistentClass)
{
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, /*class_id*/999, 0};
    TimeTableProblem p({make_class(1)}, {c});
    EXPECT_FALSE(p.is_valid());
}

TEST(TimeTableProblem, ValidWhenConstraintReferencesExistingClass)
{
    ConstraintVariant c = GroupPreferenceConstraint{0, 1.0, false, 1, 0};
    TimeTableProblem p({make_class(1)}, {c});
    EXPECT_TRUE(p.is_valid());
}

// ============================================================
//  Stream output
// ============================================================

TEST(TimeTableProblem, StreamOutputDoesNotCrash)
{
    TimeTableProblem p({make_class(1)}, {});
    std::ostringstream oss;
    EXPECT_NO_THROW(oss << p);
    EXPECT_FALSE(oss.str().empty());
}

TEST(TimeTableProblem, EmptyProblemStreamOutputDoesNotCrash)
{
    std::ostringstream oss;
    EXPECT_NO_THROW(oss << TimeTableProblem());
}

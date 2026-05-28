#include <gtest/gtest.h>
#include "time_table_state.h"

#include <sstream>

// ============================================================
//  Construction
// ============================================================

TEST(TimeTableState, DefaultConstructedIsEmpty)
{
    TimeTableState state;
    EXPECT_TRUE(state.is_empty());
    EXPECT_EQ(state.size(), 0);
}

TEST(TimeTableState, ConstructFromVector)
{
    TimeTableState state({1, 2, 3});
    EXPECT_EQ(state.size(), 3);
    EXPECT_TRUE(state.contains(1));
    EXPECT_TRUE(state.contains(2));
    EXPECT_TRUE(state.contains(3));
}

TEST(TimeTableState, ConstructFromVectorIgnoresDuplicates)
{
    TimeTableState state({7, 7, 7});
    EXPECT_EQ(state.size(), 1);
    EXPECT_TRUE(state.contains(7));
}

// ============================================================
//  add
// ============================================================

TEST(TimeTableState, AddMakesElementVisible)
{
    TimeTableState state;
    EXPECT_FALSE(state.contains(42));
    state.add(42);
    EXPECT_TRUE(state.contains(42));
}

TEST(TimeTableState, AddIncreasesSize)
{
    TimeTableState state;
    state.add(1);
    EXPECT_EQ(state.size(), 1);
    EXPECT_FALSE(state.is_empty());
}

TEST(TimeTableState, AddSameIdTwiceDoesNotDuplicate)
{
    TimeTableState state;
    state.add(5);
    state.add(5);
    EXPECT_EQ(state.size(), 1);
}

// ============================================================
//  remove
// ============================================================

TEST(TimeTableState, RemoveMakesElementInvisible)
{
    TimeTableState state({10});
    state.remove(10);
    EXPECT_FALSE(state.contains(10));
}

TEST(TimeTableState, RemoveDecreasesSize)
{
    TimeTableState state({1, 2});
    state.remove(1);
    EXPECT_EQ(state.size(), 1);
    EXPECT_TRUE(state.contains(2));
}

TEST(TimeTableState, RemoveNonExistentIdIsNoOp)
{
    TimeTableState state({1});
    state.remove(999);
    EXPECT_EQ(state.size(), 1);
    EXPECT_TRUE(state.contains(1));
}

TEST(TimeTableState, RemoveLastIdMakesStateEmpty)
{
    TimeTableState state({99});
    state.remove(99);
    EXPECT_TRUE(state.is_empty());
    EXPECT_EQ(state.size(), 0);
}

// ============================================================
//  get_chosen_ids
// ============================================================

TEST(TimeTableState, GetChosenIdsReflectsCurrentState)
{
    TimeTableState state;
    state.add(3);
    state.add(7);
    const auto& ids = state.get_chosen_ids();
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_TRUE(ids.contains(3));
    EXPECT_TRUE(ids.contains(7));
}

TEST(TimeTableState, GetChosenIdsUpdatesAfterRemove)
{
    TimeTableState state({1, 2, 3});
    state.remove(2);
    const auto& ids = state.get_chosen_ids();
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_FALSE(ids.contains(2));
}

// ============================================================
//  is_empty / size consistency
// ============================================================

TEST(TimeTableState, IsEmptyAndSizeAreConsistent)
{
    TimeTableState state;
    EXPECT_EQ(state.is_empty(), state.size() == 0);

    state.add(1);
    EXPECT_EQ(state.is_empty(), state.size() == 0);

    state.remove(1);
    EXPECT_EQ(state.is_empty(), state.size() == 0);
}

// ============================================================
//  Stream output
// ============================================================

TEST(TimeTableState, StreamOutputDoesNotCrash)
{
    TimeTableState state({3, 1, 2});
    std::ostringstream oss;
    EXPECT_NO_THROW(oss << state);
    EXPECT_FALSE(oss.str().empty());
}

TEST(TimeTableState, EmptyStateStreamOutputDoesNotCrash)
{
    TimeTableState state;
    std::ostringstream oss;
    EXPECT_NO_THROW(oss << state);
}

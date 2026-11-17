#include <gtest/gtest.h>

#include <utility>
#include <vector>

#include "dojo/unique_handle.hpp"

TEST(UniqueHandle, ClosesOnce) {
    std::vector<int> closed;
    {
        dojo::UniqueHandle handle(12, [&](int value) { closed.push_back(value); });
        EXPECT_TRUE(handle.valid());
    }
    ASSERT_EQ(closed.size(), 1u);
    EXPECT_EQ(closed[0], 12);
}

TEST(UniqueHandle, ReleaseRelinquishesOwnership) {
    int closed = 0;
    {
        dojo::UniqueHandle handle(7, [&](int) { ++closed; });
        EXPECT_EQ(handle.release(), 7);
        EXPECT_FALSE(handle.valid());
    }
    EXPECT_EQ(closed, 0);
}

TEST(UniqueHandle, ResetClosesOldAndKeepsNew) {
    std::vector<int> closed;
    {
        dojo::UniqueHandle handle(1, [&](int value) { closed.push_back(value); });
        handle.reset(2);
        EXPECT_EQ(handle.get(), 2);
    }
    ASSERT_EQ(closed.size(), 2u);
    EXPECT_EQ(closed[0], 1);
    EXPECT_EQ(closed[1], 2);
}

TEST(UniqueHandle, ResetToSameValueDoesNotDoubleClose) {
    int closed = 0;
    {
        dojo::UniqueHandle handle(5, [&](int) { ++closed; });
        handle.reset(5);
    }
    EXPECT_EQ(closed, 1);
}

TEST(UniqueHandle, MoveTransfersOwnership) {
    int closed = 0;
    {
        dojo::UniqueHandle source(3, [&](int) { ++closed; });
        dojo::UniqueHandle target(std::move(source));
        EXPECT_FALSE(source.valid());
        EXPECT_EQ(target.get(), 3);
    }
    EXPECT_EQ(closed, 1);
}

TEST(UniqueHandle, MoveAssignmentClosesPrevious) {
    int first = 0;
    int second = 0;
    {
        dojo::UniqueHandle source(1, [&](int) { ++first; });
        dojo::UniqueHandle target(2, [&](int) { ++second; });
        target = std::move(source);
        EXPECT_EQ(second, 1);
    }
    EXPECT_EQ(first, 1);
    EXPECT_EQ(second, 1);
}

TEST(UniqueHandle, MoveFromReleasedIsInert) {
    int closed = 0;
    {
        dojo::UniqueHandle source(9, [&](int) { ++closed; });
        source.release();
        dojo::UniqueHandle target(std::move(source));
        EXPECT_FALSE(target.valid());
    }
    EXPECT_EQ(closed, 0);
}

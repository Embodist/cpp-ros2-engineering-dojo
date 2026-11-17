#include <gtest/gtest.h>

#include <cstdint>
#include <utility>

#include "common/new_counter.hpp"
#include "dojo/move_buffer.hpp"

TEST(MoveOnlyBuffer, ZeroInitAndBounds) {
    dojo::MoveOnlyBuffer buffer(4);
    ASSERT_EQ(buffer.size(), 4u);
    EXPECT_EQ(buffer.read(3), 0u);
    buffer.write(3, 42);
    EXPECT_EQ(buffer.read(3), 42u);
    EXPECT_THROW(buffer.write(4, 3), std::out_of_range);
    EXPECT_THROW(buffer.read(4), std::out_of_range);
}

TEST(MoveOnlyBuffer, ResizePreservesAndZeroFills) {
    dojo::MoveOnlyBuffer buffer(2);
    buffer.write(0, 9);
    buffer.resize(5);
    ASSERT_EQ(buffer.size(), 5u);
    EXPECT_EQ(buffer.read(0), 9u);
    EXPECT_EQ(buffer.read(4), 0u);
}

TEST(MoveOnlyBuffer, ResizeZeroDropsContent) {
    dojo::MoveOnlyBuffer buffer(3);
    buffer.write(0, 1);
    buffer.resize(0);
    EXPECT_EQ(buffer.size(), 0u);
    EXPECT_THROW(buffer.read(0), std::out_of_range);
}

TEST(MoveOnlyBuffer, MoveEmptiesSource) {
    dojo::MoveOnlyBuffer source(2);
    source.write(0, 9);
    dojo::MoveOnlyBuffer target(std::move(source));
    EXPECT_EQ(source.size(), 0u);
    EXPECT_EQ(target.size(), 2u);
    EXPECT_EQ(target.read(0), 9u);
}

TEST(MoveOnlyBuffer, NewCounterSeesAllocation) {
    const long before = dojo::test::NewCounter::allocs();
    dojo::MoveOnlyBuffer buffer(64);
    const long after = dojo::test::NewCounter::allocs();
    EXPECT_EQ(buffer.size(), 64u);
    EXPECT_GE(after - before, 1);
}

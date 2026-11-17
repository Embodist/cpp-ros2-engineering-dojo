#include <gtest/gtest.h>

#include <memory>

#include "common/move_only.hpp"
#include "dojo/ring_buffer.hpp"

TEST(RingBuffer, FullAndWrap) {
    dojo::RingBuffer<int, 2> buffer;
    EXPECT_TRUE(buffer.try_push(1));
    EXPECT_TRUE(buffer.try_push(2));
    EXPECT_FALSE(buffer.try_push(3));
    EXPECT_EQ(buffer.try_pop().value(), 1);
    EXPECT_TRUE(buffer.try_push(4));
    EXPECT_EQ(buffer.try_pop().value(), 2);
    EXPECT_EQ(buffer.try_pop().value(), 4);
    EXPECT_FALSE(buffer.try_pop().has_value());
}

TEST(RingBuffer, MoveOnlyElements) {
    dojo::RingBuffer<dojo::test::MoveOnly, 1> buffer;
    EXPECT_TRUE(buffer.try_push(dojo::test::MoveOnly(8)));
    auto item = buffer.try_pop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->value(), 8);
}

TEST(RingBuffer, CapacityOneWrapsRepeatedly) {
    dojo::RingBuffer<int, 1> buffer;
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(buffer.try_push(i));
        EXPECT_FALSE(buffer.try_push(i + 100));
        EXPECT_EQ(buffer.try_pop().value(), i);
        EXPECT_FALSE(buffer.try_pop().has_value());
    }
}

TEST(RingBuffer, FullBufferDoesNotOverwrite) {
    dojo::RingBuffer<int, 2> buffer;
    ASSERT_TRUE(buffer.try_push(1));
    ASSERT_TRUE(buffer.try_push(2));
    EXPECT_FALSE(buffer.try_push(3));
    EXPECT_EQ(buffer.try_pop().value(), 1);
    EXPECT_EQ(buffer.try_pop().value(), 2);
}

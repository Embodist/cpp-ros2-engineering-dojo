#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <future>
#include <thread>

#include "common/move_only.hpp"
#include "dojo/bounded_queue.hpp"

using namespace std::chrono_literals;

TEST(BoundedQueue, FifoAndClose) {
    dojo::BoundedQueue<int> queue(2);
    EXPECT_TRUE(queue.push(10));
    EXPECT_TRUE(queue.push(11));
    queue.close();
    EXPECT_FALSE(queue.push(12));
    ASSERT_TRUE(queue.pop().has_value());
    EXPECT_EQ(queue.pop().value(), 11);
    EXPECT_FALSE(queue.pop().has_value());
    EXPECT_TRUE(queue.is_closed());
}

TEST(BoundedQueue, RejectsZeroCapacity) {
    EXPECT_THROW(dojo::BoundedQueue<int>(0), std::invalid_argument);
}

TEST(BoundedQueue, DrainsAcceptedItemsAfterClose) {
    dojo::BoundedQueue<int> queue(4);
    EXPECT_TRUE(queue.push(1));
    EXPECT_TRUE(queue.push(2));
    queue.close();
    EXPECT_EQ(queue.pop().value(), 1);
    EXPECT_EQ(queue.pop().value(), 2);
    EXPECT_FALSE(queue.pop().has_value());
}

TEST(BoundedQueue, MoveOnlyPayload) {
    dojo::BoundedQueue<dojo::test::MoveOnly> queue(1);
    EXPECT_TRUE(queue.push(dojo::test::MoveOnly(5)));
    auto item = queue.pop();
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ(item->value(), 5);
}

TEST(BoundedQueue, ConcurrentSingleProducer) {
    dojo::BoundedQueue<int> queue(1);
    auto producer = std::async(std::launch::async, [&] {
        for (int i = 0; i < 100; ++i) {
            if (!queue.push(i)) return false;
        }
        queue.close();
        return true;
    });

    for (int i = 0; i < 100; ++i) {
        auto item = queue.pop();
        ASSERT_TRUE(item.has_value());
        EXPECT_EQ(*item, i);
    }
    ASSERT_EQ(producer.wait_for(2s), std::future_status::ready);
    EXPECT_TRUE(producer.get());
}

TEST(BoundedQueue, CloseWakesBlockedProducer) {
    dojo::BoundedQueue<int> queue(1);
    ASSERT_TRUE(queue.push(1));

    std::atomic<bool> started{false};
    auto producer = std::async(std::launch::async, [&] {
        started.store(true, std::memory_order_release);
        return queue.push(2);
    });

    while (!started.load(std::memory_order_acquire)) std::this_thread::yield();
    std::this_thread::sleep_for(20ms);
    queue.close();
    EXPECT_FALSE(producer.get());
}

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

#include "dojo/callback_registry.hpp"

TEST(CallbackRegistry, OrderedSubscriptions) {
    dojo::CallbackRegistry registry;
    std::vector<int> seen;
    auto first = registry.subscribe([&](int value) { seen.push_back(value); });
    registry.subscribe([&](int value) { seen.push_back(value + 1); });

    registry.publish(9);
    ASSERT_EQ(seen.size(), 2u);
    EXPECT_EQ(seen[0], 9);
    EXPECT_EQ(seen[1], 10);
    EXPECT_TRUE(registry.unsubscribe(first));
    EXPECT_FALSE(registry.unsubscribe(first));
}

TEST(CallbackRegistry, SelfUnsubscribeAndSnapshot) {
    dojo::CallbackRegistry registry;
    int calls = 0;
    dojo::CallbackRegistry::Token token = 0;
    token = registry.subscribe([&](int) {
        ++calls;
        registry.unsubscribe(token);
        registry.subscribe([&](int) { calls += 10; });
    });

    registry.publish(1);
    EXPECT_EQ(calls, 1);
    registry.publish(2);
    EXPECT_EQ(calls, 11);
}

TEST(CallbackRegistry, RejectsEmptyCallback) {
    dojo::CallbackRegistry registry;
    EXPECT_THROW(registry.subscribe({}), std::invalid_argument);
}

TEST(CallbackRegistry, CallbackExceptionPropagatesWithoutLocking) {
    dojo::CallbackRegistry registry;
    auto token = registry.subscribe([](int) { throw std::runtime_error("callback failure"); });
    EXPECT_THROW(registry.publish(1), std::runtime_error);
    EXPECT_TRUE(registry.unsubscribe(token));
    EXPECT_NO_THROW(registry.publish(2));
}

TEST(CallbackRegistry, CallbackUnsubscribesAnotherForNextRound) {
    dojo::CallbackRegistry registry;
    int first_calls = 0;
    int second_calls = 0;
    registry.subscribe([&](int) { ++first_calls; });
    auto second = registry.subscribe([&](int) { ++second_calls; });
    registry.subscribe([&](int) { registry.unsubscribe(second); });

    registry.publish(1);
    EXPECT_EQ(first_calls, 1);
    EXPECT_EQ(second_calls, 1);
    registry.publish(2);
    EXPECT_EQ(first_calls, 2);
    EXPECT_EQ(second_calls, 1);
}

TEST(CallbackRegistry, ConcurrentSubscribePublishUnsubscribe) {
    dojo::CallbackRegistry registry;
    std::atomic<int> calls{0};

    auto worker = [&] {
        for (int i = 0; i < 100; ++i) {
            auto token = registry.subscribe(
                [&](int) { calls.fetch_add(1, std::memory_order_relaxed); });
            registry.publish(i);
            registry.unsubscribe(token);
        }
    };

    std::thread first(worker);
    std::thread second(worker);
    first.join();
    second.join();
    EXPECT_GT(calls.load(), 0);
}

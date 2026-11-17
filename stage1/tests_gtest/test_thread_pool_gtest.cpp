#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <vector>

#include "dojo/thread_pool.hpp"

TEST(ThreadPool, RunAndReturn) {
    dojo::ThreadPool pool(2);
    auto a = pool.submit([] { return 6; });
    auto b = pool.submit([] { return 7; });
    EXPECT_EQ(a.get() + b.get(), 13);
    pool.shutdown();
}

TEST(ThreadPool, ExceptionPropagation) {
    dojo::ThreadPool pool(1);
    auto future = pool.submit([]() -> int { throw std::runtime_error("boom"); });
    EXPECT_THROW(future.get(), std::runtime_error);
}

TEST(ThreadPool, RejectAfterShutdown) {
    dojo::ThreadPool pool(1);
    pool.shutdown();
    EXPECT_THROW(pool.submit([] { return 1; }), std::runtime_error);
}

TEST(ThreadPool, ZeroWorkersThrows) {
    EXPECT_THROW(dojo::ThreadPool(0), std::invalid_argument);
}

TEST(ThreadPool, ShutdownDrainsQueuedTasks) {
    dojo::ThreadPool pool(1);
    std::atomic<int> done{0};
    for (int i = 0; i < 5; ++i) {
        auto future = pool.submit([&] {
            done.fetch_add(1, std::memory_order_relaxed);
            return 0;
        });
        (void)future;
    }
    pool.shutdown();
    EXPECT_EQ(done.load(), 5);
}

TEST(ThreadPool, ManyTasksAcrossWorkers) {
    dojo::ThreadPool pool(4);
    std::atomic<int> sum{0};
    std::vector<std::future<int>> futures;
    for (int i = 0; i < 100; ++i) {
        futures.push_back(pool.submit([&] {
            sum.fetch_add(1, std::memory_order_relaxed);
            return 1;
        }));
    }
    for (auto& future : futures) EXPECT_EQ(future.get(), 1);
    EXPECT_EQ(sum.load(), 100);
}

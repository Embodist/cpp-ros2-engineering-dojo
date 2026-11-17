// 项目 8 参考测试：BlockingQueue + ThreadPool
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <thread>
#include <vector>

#include "blocking_queue.hpp"
#include "thread_pool.hpp"

// ============================ BlockingQueue 基本语义 ============================
TEST(BlockingQueue, BoundedTryPushRespectsCapacity) {
    BlockingQueue<int> q(2);
    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_FALSE(q.try_push(3));                  // 满 → 拒绝
    int x = 0;
    EXPECT_TRUE(q.try_pop(x));
    EXPECT_EQ(x, 1);
    EXPECT_TRUE(q.try_push(3));                   // 腾出空位后成功
    EXPECT_EQ(q.size(), 2u);
}

TEST(BlockingQueue, CloseDrainsRemainingThenReturnsFalse) {
    BlockingQueue<int> q;
    q.push(1);
    q.push(2);
    q.close();

    std::vector<int> got;
    int x = 0;
    while (q.pop(x)) got.push_back(x);            // 关闭后仍把剩余元素取完
    EXPECT_EQ(got, (std::vector<int>{1, 2}));

    EXPECT_FALSE(q.pop(x));                        // 关闭且空 → 立即 false，不阻塞
    EXPECT_TRUE(q.closed());
    EXPECT_THROW(q.push(3), std::runtime_error);   // 关闭后 push 抛错（显式语义）
}

TEST(BlockingQueue, PopBlocksUntilElementArrives) {
    BlockingQueue<int> q;
    std::atomic<bool> got{false};
    int received = 0;
    std::thread consumer([&] {
        int v = 0;
        if (q.pop(v)) { received = v; got.store(true); }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    EXPECT_FALSE(got.load());                      // 还没有元素 → 消费者阻塞中
    q.push(99);
    consumer.join();
    EXPECT_TRUE(got.load());
    EXPECT_EQ(received, 99);
}

// ============================ 压力：多生产 / 多消费 ============================
TEST(BlockingQueue, MultiProducerMultiConsumerStress) {
    BlockingQueue<int> q;
    constexpr int kProducers = 4;
    constexpr int kPerProducer = 20000;
    std::atomic<long long> sum{0};
    std::atomic<int>       count{0};

    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p)
        producers.emplace_back([&q] {
            for (int i = 1; i <= kPerProducer; ++i) q.push(i);
        });

    std::vector<std::thread> consumers;
    for (int c = 0; c < 4; ++c)
        consumers.emplace_back([&] {
            int v = 0;
            while (q.pop(v)) { sum.fetch_add(v); count.fetch_add(1); }
        });

    for (auto& t : producers) t.join();
    q.close();                                     // 生产完 → 关闭，消费者取空后退出
    for (auto& t : consumers) t.join();

    const long long per = static_cast<long long>(kPerProducer) * (kPerProducer + 1) / 2;
    EXPECT_EQ(sum.load(), per * kProducers);       // 元素总和一致
    EXPECT_EQ(count.load(), kProducers * kPerProducer);   // 个数一致（无丢无重）
}

// ============================ ThreadPool ============================
TEST(ThreadPool, RunsAllTasksAndReturnsResults) {
    ThreadPool pool(4);
    std::vector<std::future<int>> futs;
    futs.reserve(1000);
    for (int i = 0; i < 1000; ++i) futs.push_back(pool.submit([i] { return i * i; }));

    long long got = 0;
    for (auto& f : futs) got += f.get();
    long long expect = 0;
    for (int i = 0; i < 1000; ++i) expect += static_cast<long long>(i) * i;
    EXPECT_EQ(got, expect);
}

TEST(ThreadPool, ExceptionPropagatesThroughFutureAndPoolSurvives) {
    ThreadPool pool(2);
    auto bad = pool.submit([]() -> int { throw std::runtime_error("boom"); });
    EXPECT_THROW(bad.get(), std::runtime_error);   // 异常经 future 传回

    auto good = pool.submit([] { return 42; });    // worker 没被打死，池仍可用
    EXPECT_EQ(good.get(), 42);
}

TEST(ThreadPool, SubmitFromWithinTaskDoesNotDeadlock) {
    ThreadPool pool(2);                            // 至少 2 线程：一个跑 outer，一个跑 inner
    auto outer = pool.submit([&pool] {
        auto inner = pool.submit([] { return 7; });
        return inner.get() + 1;
    });
    EXPECT_EQ(outer.get(), 8);
}

TEST(ThreadPool, ArgumentsAreForwardedAndMoveOnlyWorks) {
    ThreadPool pool(2);
    auto f = pool.submit([](std::unique_ptr<int> p, int add) { return *p + add; },
                         std::make_unique<int>(10), 5);
    EXPECT_EQ(f.get(), 15);
}

TEST(ThreadPool, DestructorDrainsPendingTasks) {
    std::atomic<int> counter{0};
    {
        ThreadPool pool(4);
        for (int i = 0; i < 200; ++i)
            pool.submit([&counter] {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                counter.fetch_add(1);
            });
    }                                              // 析构 = 优雅关闭：剩余任务全部跑完
    EXPECT_EQ(counter.load(), 200);
}

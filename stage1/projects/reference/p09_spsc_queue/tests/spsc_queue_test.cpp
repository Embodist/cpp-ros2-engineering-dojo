// 项目 9 参考测试：SpscQueue<T, N>
#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <thread>
#include <vector>

#include "common/tracer.hpp"
#include "spsc_queue.hpp"

// ============================ 基本语义 / 满空判定 ============================
TEST(SpscQueue, EmptyFullAndSizeApprox) {
    SpscQueue<int, 4> q;
    EXPECT_TRUE(q.empty());
    EXPECT_EQ(q.size_approx(), 0u);

    EXPECT_TRUE(q.try_push(1));
    EXPECT_TRUE(q.try_push(2));
    EXPECT_EQ(q.size_approx(), 2u);
    EXPECT_FALSE(q.empty());

    int v = 0;
    EXPECT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 1);
    EXPECT_EQ(q.size_approx(), 1u);
}

TEST(SpscQueue, WrapAroundReusesSlots) {
    SpscQueue<int, 4> q;                              // 容量 4
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.try_push(i));
    EXPECT_FALSE(q.try_push(9));                      // 满

    int v = 0;
    for (int i = 0; i < 4; ++i) { EXPECT_TRUE(q.try_pop(v)); EXPECT_EQ(v, i); }
    EXPECT_FALSE(q.try_pop(v));                       // 空

    // 再压一轮 → 下标回绕，槽位被复用（计数器从 4 涨到 8，索引 & 3 回到 0..3）
    for (int i = 0; i < 4; ++i) EXPECT_TRUE(q.try_push(100 + i));
    for (int i = 0; i < 4; ++i) { EXPECT_TRUE(q.try_pop(v)); EXPECT_EQ(v, 100 + i); }
}

// ============================ 非平凡 / move-only 类型 ============================
TEST(SpscQueue, NonTrivialAndMoveOnlyType) {
    SpscQueue<std::unique_ptr<int>, 4> q;
    EXPECT_TRUE(q.try_push(std::make_unique<int>(1)));
    EXPECT_TRUE(q.try_push(std::make_unique<int>(2)));

    std::unique_ptr<int> p;
    EXPECT_TRUE(q.try_pop(p));
    EXPECT_EQ(*p, 1);
    EXPECT_TRUE(q.try_pop(p));
    EXPECT_EQ(*p, 2);
    EXPECT_FALSE(q.try_pop(p));
}

TEST(SpscQueue, DestructorCleansRemainingElements) {
    Tracer::reset();
    {
        SpscQueue<Tracer, 8> q;
        for (int i = 0; i < 5; ++i) q.try_push(Tracer(i));
        // 故意不取空，直接析构 → 剩余 5 个槽位里的元素必须被正确析构
    }
    EXPECT_TRUE(Tracer::balanced());                  // live == 0：无泄漏（ASan 再兜底）
    EXPECT_EQ(Tracer::dtor, Tracer::total_ctor());    // 构造数 == 析构数
}

// ============================ 单线程顺序正确性 ============================
TEST(SpscQueue, FifoOrderSingleThread) {
    SpscQueue<int, 16> q;
    int next_push = 0, next_pop = 0, v = 0;
    for (int step = 0; step < 1000; ++step) {
        if (q.size_approx() < 8) {                    // 保持未满，交替压入 / 弹出
            EXPECT_TRUE(q.try_push(next_push));
            ++next_push;
        } else {
            EXPECT_TRUE(q.try_pop(v));
            EXPECT_EQ(v, next_pop);
            ++next_pop;
        }
    }
    while (q.try_pop(v)) { EXPECT_EQ(v, next_pop); ++next_pop; }
    EXPECT_EQ(next_push, next_pop);
}

// ============================ 百万级压力：顺序与内容一致 ============================
TEST(SpscQueue, ProducerConsumerMillionElements) {
    constexpr std::size_t kTotal = 1'000'000;
    SpscQueue<std::size_t, 1024> q;
    std::atomic<bool> ok{true};

    std::thread producer([&] {
        for (std::size_t i = 0; i < kTotal;) {
            if (q.try_push(i)) ++i;
        }
    });
    std::thread consumer([&] {
        std::size_t expect = 0, v = 0;
        while (expect < kTotal) {
            if (q.try_pop(v)) {
                if (v != expect) ok.store(false);
                ++expect;
            }
        }
    });

    producer.join();
    consumer.join();
    EXPECT_TRUE(ok.load());                           // 顺序与内容完全一致
    EXPECT_TRUE(q.empty());
}

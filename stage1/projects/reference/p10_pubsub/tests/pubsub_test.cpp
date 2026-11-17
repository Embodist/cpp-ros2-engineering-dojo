// 项目 10 参考测试：迷你发布订阅 + 执行器 + 定时器
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "common/tracer.hpp"
#include "pubsub.hpp"

using namespace std::chrono_literals;

namespace {
using IntTopic = mini::Topic<int>;
using IntPtr   = std::shared_ptr<const int>;
}

// ============================ 基本发布 / 订阅 ============================
TEST(PubSub, BasicPublishSubscribe) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);

    std::vector<int> got;
    mini::Subscription<int> sub(topic, topic->subscribe([&](const IntPtr& m) { got.push_back(*m); }));

    pub.publish(1);
    pub.publish(2);
    EXPECT_TRUE(got.empty());                     // 未 spin，回调还没执行
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{1, 2}));
}

// ============================ RAII 退订 ============================
TEST(PubSub, SubscriptionHandleUnsubscribesOnDestruction) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::vector<int> got;

    {
        mini::Subscription<int> sub(topic, topic->subscribe([&](const IntPtr& m) { got.push_back(*m); }));
        pub.publish(1);
        topic->drain_once();
        EXPECT_EQ(got, (std::vector<int>{1}));
    }                                             // 句柄析构 → 自动退订

    EXPECT_EQ(topic->subscriber_count(), 0u);
    pub.publish(2);
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{1}));        // 不再收到回调
}

// ============================ 回调执行期间被退订 ============================
TEST(PubSub, UnsubscribeDuringCallbackIsSafe) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::vector<int> got;

    mini::Subscription<int> sub;                  // 先占位，回调需要引用它
    sub = mini::Subscription<int>(topic, topic->subscribe([&](const IntPtr& m) {
        got.push_back(*m);
        sub.reset();                              // 回调里退订自己
    }));

    pub.publish(1);
    topic->drain_once();                          // 不能崩溃
    EXPECT_EQ(got, (std::vector<int>{1}));

    pub.publish(2);
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{1}));        // 已退订，不再回调
}

// ============================ 回调里再 publish 不死锁 ============================
TEST(PubSub, PublishFromCallbackDoesNotDeadlock) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::vector<int> got;

    mini::Subscription<int> sub(topic, topic->subscribe([&](const IntPtr& m) {
        if (*m < 3) pub.publish(*m + 1);          // 回调里再发布
        got.push_back(*m);
    }));

    pub.publish(0);
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{0, 1, 2, 3}));  // 链式投递：0→1→2→3，无死锁
}

// ============================ 消息生命周期：最后一个处理完才释放 ============================
TEST(PubSub, MessageLifetimeSharedZeroCopy) {
    Tracer::reset();
    auto topic = std::make_shared<mini::Topic<Tracer>>();
    mini::Publisher<Tracer> pub(topic);
    int a = 0, b = 0;

    auto s1 = topic->subscribe([&](const std::shared_ptr<const Tracer>& m) { a = m->value(); });
    auto s2 = topic->subscribe([&](const std::shared_ptr<const Tracer>& m) { b = m->value(); });

    auto msg = std::make_shared<const Tracer>(7);
    std::weak_ptr<const Tracer> wk = msg;
    pub.publish(msg);
    topic->drain_once();

    EXPECT_EQ(a, 7);
    EXPECT_EQ(b, 7);
    EXPECT_EQ(Tracer::copy_ctor, 0);              // 两个订阅者共享同一份消息，零元素拷贝
    msg.reset();                                  // 发布方也放开
    EXPECT_TRUE(wk.expired());                    // 处理完即释放
}

// ============================ 队列深度 + 丢弃策略 ============================
TEST(PubSub, DropOldestPolicy) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::vector<int> got;
    auto s = topic->subscribe([&](const IntPtr& m) { got.push_back(*m); },
                              2, mini::DropPolicy::DropOldest);
    for (int i = 0; i < 4; ++i) pub.publish(i);   // 深度 2 → 只保留最新的 2 个
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{2, 3}));
}

TEST(PubSub, DropNewestPolicy) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::vector<int> got;
    auto s = topic->subscribe([&](const IntPtr& m) { got.push_back(*m); },
                              2, mini::DropPolicy::DropNewest);
    for (int i = 0; i < 4; ++i) pub.publish(i);   // 深度 2 → 保留最早的 2 个
    topic->drain_once();
    EXPECT_EQ(got, (std::vector<int>{0, 1}));
}

// ============================ 多线程执行器 ============================
TEST(PubSub, MultiThreadedExecutorProcessesAll) {
    auto topic = std::make_shared<IntTopic>();
    mini::Publisher<int> pub(topic);
    std::atomic<int> cnt{0};
    auto sub = topic->subscribe([&](const IntPtr&) { cnt.fetch_add(1); });

    for (int i = 0; i < 1000; ++i) pub.publish(i);

    mini::Executor ex(4);
    ex.add_topic(topic);
    std::thread runner([&] { ex.spin(); });
    while (cnt.load() < 1000) std::this_thread::yield();
    ex.stop();
    runner.join();

    EXPECT_EQ(cnt.load(), 1000);                  // 4 线程并发 drain，不丢不重（TSan 检查数据竞争）
}

// ============================ 定时器 ============================
TEST(PubSub, TimerFiresRepeatedly) {
    std::atomic<int> ticks{0};
    auto timer = std::make_shared<mini::Timer>(10ms, [&] { ticks.fetch_add(1); });

    mini::Executor ex(1);
    ex.add_timer(timer);
    std::thread runner([&] { ex.spin(); });
    std::this_thread::sleep_for(120ms);
    ex.stop();
    runner.join();

    // 120ms / 10ms ≈ 12 次；给宽松上下界（调度抖动、CI 负载）。
    EXPECT_GE(ticks.load(), 5);
    EXPECT_LE(ticks.load(), 40);
}

TEST(PubSub, TimerCancelStopsCallbacks) {
    std::atomic<int> ticks{0};
    auto timer = std::make_shared<mini::Timer>(5ms, [&] { ticks.fetch_add(1); });
    mini::Executor ex(1);
    ex.add_timer(timer);
    std::thread runner([&] { ex.spin(); });
    std::this_thread::sleep_for(30ms);
    timer->cancel();
    const int after_cancel = ticks.load();
    std::this_thread::sleep_for(30ms);
    ex.stop();
    runner.join();
    EXPECT_LE(ticks.load() - after_cancel, 1);    // 取消后基本不再触发
}

#pragma once
// 迷你发布订阅 + 执行器 + 定时器（项目 10）—— 对照 rclcpp 的思路
// ============================================================================
// 部件对应关系（与 rclcpp 对照）：
//   Topic<Msg>        ~ 话题（内部维护订阅者列表）
//   Publisher<Msg>    ~ rclcpp::Publisher
//   Subscription<Msg> ~ rclcpp::Subscription（RAII：析构自动退订）
//   Subscriber<Msg>   ~ 订阅者的回调队列（深度 + 丢弃策略）
//   Executor          ~ SingleThreadedExecutor / MultiThreadedExecutor
//   Timer             ~ rclcpp::create_wall_timer
//
// 关键设计（都对应核查点）：
//   * 消息以 shared_ptr<const Msg> 传递 —— 多个订阅者【零拷贝】共享同一份消息，
//     最后一个处理完才释放（引用计数）。
//   * publish 只在锁内【快照】订阅者列表，投递与回调执行都在锁外 —— 所以回调里
//     再 publish / subscribe 不会死锁。
//   * 退订：Subscription 析构时把 Subscriber 置为非 active 并从 Topic 摘除；执行器
//     在调用回调前会检查 active，避免"回调执行期间被退订"导致调用已退订的回调。
// ============================================================================

#include <atomic>
#include <chrono>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

namespace mini {

using Clock = std::chrono::steady_clock;

// 队列满时的丢弃策略。
enum class DropPolicy { DropOldest, DropNewest };

// 订阅者：一个带深度限制的回调队列。
template <class Msg>
class Subscriber {
public:
    using MsgPtr   = std::shared_ptr<const Msg>;
    using Callback = std::function<void(const MsgPtr&)>;

    Subscriber(Callback cb, std::size_t depth, DropPolicy policy)
        : cb_(std::move(cb)), depth_(depth == 0 ? static_cast<std::size_t>(-1) : depth), policy_(policy) {}

    // 投递一条消息（生产者/发布者线程调用）。
    void deliver(const MsgPtr& msg) {
        std::lock_guard lk(m_);
        if (!active_) return;
        if (q_.size() >= depth_) {
            if (policy_ == DropPolicy::DropOldest) q_.pop_front();   // 丢最旧，给新消息腾位
            else return;                                             // 丢最新：直接拒绝
        }
        q_.push_back(msg);
    }

    // 取出一个待处理消息（执行器线程调用）。
    bool take(MsgPtr& out) {
        std::lock_guard lk(m_);
        if (q_.empty()) return false;
        out = q_.front();
        q_.pop_front();
        return true;
    }

    std::size_t pending() const { std::lock_guard lk(m_); return q_.size(); }
    bool        active()  const noexcept { return active_.load(); }
    void        deactivate() noexcept { active_.store(false); }

    const Callback& callback() const noexcept { return cb_; }

private:
    Callback                       cb_;
    std::size_t                    depth_;
    DropPolicy                     policy_;
    mutable std::mutex             m_;
    std::deque<MsgPtr>             q_;
    std::atomic<bool>              active_{true};
};

template <class Msg>
class Topic {
public:
    using MsgPtr     = std::shared_ptr<const Msg>;
    using SubPtr     = std::shared_ptr<Subscriber<Msg>>;
    using Callback   = typename Subscriber<Msg>::Callback;

    Topic() = default;
    Topic(const Topic&)            = delete;
    Topic& operator=(const Topic&) = delete;

    SubPtr subscribe(Callback cb, std::size_t depth = 0, DropPolicy policy = DropPolicy::DropOldest) {
        auto sub = std::make_shared<Subscriber<Msg>>(std::move(cb), depth, policy);
        std::lock_guard lk(m_);
        subs_.push_back(sub);
        return sub;
    }

    void unsubscribe(const SubPtr& sub) {
        std::lock_guard lk(m_);
        sub->deactivate();
        for (auto it = subs_.begin(); it != subs_.end(); ++it) {
            if (*it == sub) { subs_.erase(it); break; }
        }
    }

    void publish(const MsgPtr& msg) {
        // 易错点: 只在锁内复制【订阅者列表】，投递在锁外做。若在持锁时投递，而投递又
        //         触及订阅者的锁，就可能与"回调里再 publish"形成锁序死锁。
        std::vector<SubPtr> snapshot;
        {
            std::lock_guard lk(m_);
            snapshot = subs_;
        }
        for (auto& s : snapshot) s->deliver(msg);
    }

    std::size_t subscriber_count() const { std::lock_guard lk(m_); return subs_.size(); }

    // 执行器调用：把所有订阅者的待处理回调跑一轮，返回执行的回调数。
    std::size_t drain_once() {
        std::vector<SubPtr> snapshot;
        {
            std::lock_guard lk(m_);
            snapshot = subs_;
        }
        std::size_t n = 0;
        for (auto& s : snapshot) {
            MsgPtr msg;
            while (s->active() && s->take(msg)) {          // 退订后立即停止处理
                s->callback()(msg);                        // 回调里可能再 publish/subscribe
                ++n;
            }
        }
        return n;
    }

private:
    mutable std::mutex  m_;
    std::vector<SubPtr> subs_;
};

template <class Msg>
class Publisher {
public:
    using MsgPtr = std::shared_ptr<const Msg>;
    explicit Publisher(std::shared_ptr<Topic<Msg>> t) : topic_(std::move(t)) {}

    void publish(const MsgPtr& msg)        { topic_->publish(msg); }
    void publish(const Msg& msg)           { topic_->publish(std::make_shared<const Msg>(msg)); }

private:
    std::shared_ptr<Topic<Msg>> topic_;
};

// RAII 订阅句柄：析构 / reset 时自动退订。
template <class Msg>
class Subscription {
public:
    Subscription() = default;
    Subscription(std::shared_ptr<Topic<Msg>> t, std::shared_ptr<Subscriber<Msg>> s)
        : topic_(std::move(t)), sub_(std::move(s)) {}

    Subscription(Subscription&&) noexcept            = default;
    Subscription& operator=(Subscription&&) noexcept = default;
    Subscription(const Subscription&)                = delete;
    Subscription& operator=(const Subscription&)     = delete;

    ~Subscription() { reset(); }

    void reset() {
        if (topic_ && sub_) topic_->unsubscribe(sub_);
        topic_.reset();
        sub_.reset();
    }
    bool valid() const noexcept { return static_cast<bool>(sub_); }

private:
    std::shared_ptr<Topic<Msg>>      topic_;
    std::shared_ptr<Subscriber<Msg>> sub_;
};

// 周期定时器：next_ 按 period 递推以减少漂移（累计误差不累积）。
class Timer {
public:
    using Callback = std::function<void()>;

    Timer(Clock::duration period, Callback cb)
        : period_(period), cb_(std::move(cb)), next_(Clock::now() + period) {}

    bool active() const noexcept { return active_.load(); }
    void cancel() noexcept { active_.store(false); }
    Clock::duration period() const noexcept { return period_; }

    bool due(Clock::time_point now) const noexcept { return now >= next_; }

    // 到点后调用：先按 period 递推下一次（挪后减少漂移），再执行回调。
    void fire(Clock::time_point now) {
        next_ += period_;
        if (next_ <= now) next_ = now + period_;          // 落后太多（被阻塞）则重置，防止追帧风暴
        cb_();
    }

private:
    Clock::duration   period_;
    Callback          cb_;
    Clock::time_point next_;
    std::atomic<bool> active_{true};
};

// 执行器：轮询所有已注册话题的待处理回调 + 触发到点的定时器。
class Executor {
public:
    explicit Executor(std::size_t threads = 1)
        : threads_(threads == 0 ? 1 : threads) {}

    template <class Msg>
    void add_topic(std::shared_ptr<Topic<Msg>> t) {
        std::lock_guard lk(m_);
        drains_.push_back([t] { return t->drain_once(); });
    }
    void add_timer(std::shared_ptr<Timer> t) {
        std::lock_guard lk(m_);
        timers_.push_back(std::move(t));
    }

    // 跑一轮：内部先复制 drainer / timer 列表（锁外执行，允许回调再 add_*）。
    std::size_t spin_once() {
        std::vector<std::function<std::size_t()>> drains;
        std::vector<std::shared_ptr<Timer>>       timers;
        {
            std::lock_guard lk(m_);
            drains = drains_;
            timers = timers_;
        }
        std::size_t executed = 0;
        for (auto& d : drains) executed += d();
        const auto now = Clock::now();
        for (auto& t : timers) if (t->active() && t->due(now)) t->fire(now);
        return executed;
    }

    // 单线程：本线程循环；多线程：起 threads_ 个线程一起 spin_once。
    void spin() {
        if (threads_ <= 1) {
            while (!stop_.load()) spin_once();
            return;
        }
        std::vector<std::thread> pool;
        for (std::size_t i = 1; i < threads_; ++i)
            pool.emplace_back([this] { while (!stop_.load()) spin_once(); });
        while (!stop_.load()) spin_once();
        for (auto& th : pool) th.join();
    }

    void stop() noexcept { stop_.store(true); }

private:
    std::mutex                                 m_;
    std::size_t                                threads_;
    std::vector<std::function<std::size_t()>>  drains_;
    std::vector<std::shared_ptr<Timer>>        timers_;
    std::atomic<bool>                          stop_{false};
};

}  // namespace mini

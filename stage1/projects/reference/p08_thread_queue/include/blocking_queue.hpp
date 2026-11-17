#pragma once
// BlockingQueue<T> —— 线程安全阻塞队列（项目 8）
// ============================================================================
// 训练点：mutex + condition_variable、虚假唤醒、关闭语义、异常传播。
//
// 关闭语义（务必写死并测试）：close() 只置关闭位并唤醒所有等待者，不丢元素。
//   关闭后：pop 仍会把队列里剩余元素逐个取走，取空后才返回 false；
//           push 则抛 runtime_error（也可以设计成丢弃，但必须显式）。
// ============================================================================

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>

template <class T>
class BlockingQueue {
public:
    // capacity == 0 表示无界。
    explicit BlockingQueue(std::size_t capacity = 0) : capacity_(capacity) {}
    BlockingQueue(const BlockingQueue&)            = delete;
    BlockingQueue& operator=(const BlockingQueue&) = delete;

    // 阻塞式入队：有界且满则等待；队列已关闭则抛 runtime_error。
    void push(T value) {
        std::unique_lock lk(m_);
        // 易错点: 必须用 predicate 版 wait（等价于 while 循环重查条件）。裸 wait() 会被
        //         【虚假唤醒】骗醒——条件没满足就往下走，于是满队塞入 / 空队取出。
        not_full_.wait(lk, [this] { return closed_ || capacity_ == 0 || q_.size() < capacity_; });
        if (closed_) throw std::runtime_error("BlockingQueue::push on closed queue");
        q_.push_back(std::move(value));
        not_empty_.notify_one();
    }

    // 非阻塞入队：满或已关闭 → false。
    bool try_push(T value) {
        std::lock_guard lk(m_);
        if (closed_) return false;
        if (capacity_ != 0 && q_.size() >= capacity_) return false;
        q_.push_back(std::move(value));
        not_empty_.notify_one();
        return true;
    }

    // 阻塞式出队：有元素则取走返回 true；关闭且取空 → 立即返回 false（不再阻塞）。
    bool pop(T& out) {
        std::unique_lock lk(m_);
        not_empty_.wait(lk, [this] { return !q_.empty() || closed_; });
        if (q_.empty()) return false;                 // 关闭且已取空
        out = std::move(q_.front());
        q_.pop_front();
        not_full_.notify_one();
        return true;
    }

    // 非阻塞出队：空 → false。
    bool try_pop(T& out) {
        std::lock_guard lk(m_);
        if (q_.empty()) return false;
        out = std::move(q_.front());
        q_.pop_front();
        not_full_.notify_one();
        return true;
    }

    // 关闭：置位并唤醒所有等待者。不丢弃队列里已有的元素。
    void close() {
        std::lock_guard lk(m_);
        closed_ = true;
        not_empty_.notify_all();                      // 唤醒所有阻塞的消费者
        not_full_.notify_all();                       // 唤醒所有阻塞的生产者（让它们看到 closed）
    }

    bool        closed()   const { std::lock_guard lk(m_); return closed_; }
    std::size_t size()     const { std::lock_guard lk(m_); return q_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }

private:
    mutable std::mutex      m_;
    std::condition_variable not_empty_;
    std::condition_variable not_full_;
    std::deque<T>           q_;
    std::size_t             capacity_;                // 0 = 无界
    bool                    closed_ = false;
};

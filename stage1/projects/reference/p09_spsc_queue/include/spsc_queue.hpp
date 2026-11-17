#pragma once
// SpscQueue<T, N> —— 单生产者单消费者无锁环形队列（项目 9）
// ============================================================================
// 训练点：std::atomic、memory order（acquire/release）、false sharing、2 的幂容量 + 位掩码。
// 这是"传感器线程 → 控制线程"实时数据通道的基础。
//
// 为什么 SPSC 不需要 CAS：只有一个写者（生产者写 tail_）、一个读者（消费者写 head_），
//   各自独占一个下标，读对方的下标只读不改。既然没有"多方竞争写同一个变量"，就不需要
//   比较交换；release/acquire 建立 happens-before 就足够安全。
//
// head_/tail_ 用【单调递增的计数器】（不回绕到 0），下标 = 计数器 & (N-1)。
//   好处：满/空判定只用一个减法，天然处理 size_t 回绕（无符号减法本就是对 2^64 取模）。
// ============================================================================

#include <atomic>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

template <class T, std::size_t N>
class SpscQueue {
    static_assert(N >= 2, "容量至少 2");
    static_assert((N & (N - 1)) == 0, "N 必须是 2 的幂（便于用位掩码取模）");
    static_assert(alignof(T) <= 64, "为简化槽位对齐，要求 alignof(T) <= 64");

    static constexpr std::size_t kMask = N - 1;

public:
    SpscQueue() noexcept = default;
    SpscQueue(const SpscQueue&)            = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    ~SpscQueue() {
        // 析构时清理队列里剩余的元素（否则非平凡类型会泄漏）。
        std::size_t h = head_.load(std::memory_order_relaxed);
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        for (; h != t; ++h) slot(h & kMask)->~T();
    }

    // 仅生产者线程调用。
    bool try_push(const T& v) { return emplace(v); }
    bool try_push(T&& v)      { return emplace(std::move(v)); }

    // 仅消费者线程调用。成功取走返回 true。
    bool try_pop(T& out) {
        const std::size_t h = head_.load(std::memory_order_relaxed);
        std::size_t t = cached_tail_;                 // 本线程私有缓存，先不动 atomic
        if (h == t) {
            // 可能空：这时才去读生产者的 tail_。acquire 与生产者的 release 配对，
            // 保证"读到 tail_ 的新值"时，元素构造也已完成可见。
            t = tail_.load(std::memory_order_acquire);
            cached_tail_ = t;
            if (h == t) return false;
        }
        T* p = slot(h & kMask);
        out = std::move(*p);
        p->~T();
        // release：把"槽位已读空"这个事实发布给生产者，它才能安全地覆写这个槽。
        head_.store(h + 1, std::memory_order_release);
        return true;
    }

    bool empty() const noexcept {
        return head_.load(std::memory_order_acquire) == tail_.load(std::memory_order_acquire);
    }
    std::size_t size_approx() const noexcept {
        return tail_.load(std::memory_order_acquire) - head_.load(std::memory_order_acquire);
    }
    static constexpr std::size_t capacity() noexcept { return N; }

private:
    T* slot(std::size_t idx) noexcept {
        return std::launder(reinterpret_cast<T*>(buf_ + idx * sizeof(T)));
    }

    template <class U>
    bool emplace(U&& v) {
        const std::size_t t = tail_.load(std::memory_order_relaxed);
        std::size_t h = cached_head_;                 // 生产者私有缓存
        if (t - h == N) {
            // 可能满：读消费者 head_。acquire 与消费者的 release 配对。
            h = head_.load(std::memory_order_acquire);
            cached_head_ = h;
            if (t - h == N) return false;             // 真满
        }
        new (static_cast<void*>(slot(t & kMask))) T(std::forward<U>(v));
        // release：发布"新元素已就位"，消费者 acquire 到 tail_ 新值后即可安全读取。
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }

    // ---- false sharing：读下标与写下标放在不同缓存行（alignas(64)）----
    // 否则两个核心频繁改动同一缓存行，会来回抢缓存所有权（cache line ping-pong）。
    alignas(64) std::atomic<std::size_t> head_{0};    // 消费者写，生产者读
    alignas(64) std::atomic<std::size_t> tail_{0};    // 生产者写，消费者读
    // 对端下标的缓存：只被本线程读写，故用普通变量即可（也各自独占缓存行）。
    alignas(64) std::size_t cached_head_{0};          // 生产者缓存消费者的 head
    alignas(64) std::size_t cached_tail_{0};          // 消费者缓存生产者的 tail
    alignas(64) unsigned char buf_[N * sizeof(T)];    // 原始存储，就地构造/析构
};

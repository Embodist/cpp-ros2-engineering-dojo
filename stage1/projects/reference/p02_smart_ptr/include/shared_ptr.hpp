#pragma once
// SharedPtr / WeakPtr / MakeShared / 控制块（项目 2b）
// ============================================================================
// 训练点：共享所有权、控制块与引用计数、原子操作与内存序、类型擦除的删除器、
//         对象销毁与控制块释放的时机分离、MakeShared 的单次分配。
//
// 关键概念 —— 控制块（control block）：
//   它同时持有"强引用计数"和"弱引用计数"，以及（多态地）销毁对象的逻辑。
//   对象与控制块的生命周期【分开】：
//     · 强计数归零 → 销毁被管理对象，但控制块可能还被 weak_ptr 引用，暂不释放；
//     · 弱计数归零 → 释放控制块。
// ============================================================================

#include <atomic>
#include <cstddef>
#include <memory>      // std::addressof / 便于演示
#include <type_traits>
#include <utility>

#include "unique_ptr.hpp"   // DefaultDelete

template <class T> class SharedPtr;                                  // 前向声明
template <class T, class... Args> SharedPtr<T> MakeShared(Args&&...); // 前向声明

// ---------------- 控制块基类 ----------------
struct ControlBlockBase {
    // 易错点: 计数必须是 atomic。多线程拷贝/销毁同一对象时，普通 int 会数据竞争（UB）。
    std::atomic<long> strong{1};
    std::atomic<long> weak{1};   // 注意：初值 1，代表"被强引用隐式持有的那一个弱引用"

    ControlBlockBase() = default;
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;
    virtual ~ControlBlockBase() = default;

    void add_strong() noexcept { strong.fetch_add(1, std::memory_order_relaxed); }
    void add_weak()   noexcept { weak.fetch_add(1, std::memory_order_relaxed); }

    // 释放一个强引用
    void release_strong() noexcept {
        // 取舍: 减到 0 用 acq_rel —— release 保证此线程对对象的写入对"最后一个析构者"可见，
        //       acquire 保证能看到其它线程的写入。计数本身用 relaxed 增量即可。
        if (strong.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            destroy_object();          // 强引用没了 → 销毁对象
            release_weak();            // 并释放隐式的那个弱引用
        }
    }

    // 释放一个弱引用
    void release_weak() noexcept {
        if (weak.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            deallocate_block();        // 最后一个弱引用没了 → 释放控制块
        }
    }

    virtual void destroy_object() noexcept = 0;
    virtual void deallocate_block() noexcept { delete this; }
};

// 控制块之 PtrControlBlock：对象在别处，块里放指针 + 删除器
// 用途: SharedPtr(new T) 或带自定义删除器
template <class T, class D>
struct PtrControlBlock : ControlBlockBase {
    T* ptr;
    [[no_unique_address]] D deleter;

    template <class DD>
    PtrControlBlock(T* p, DD&& d) : ptr(p), deleter(std::forward<DD>(d)) {}

    void destroy_object() noexcept override { deleter(ptr); }
};

// 控制块之 EmbeddedControlBlock：对象【内嵌】在块中（MakeShared 用）
// 一次分配同时放下控制块与对象 → 省一次 malloc，且缓存更友好。
template <class T>
struct EmbeddedControlBlock : ControlBlockBase {
    alignas(T) unsigned char storage[sizeof(T)];

    template <class... Args>
    explicit EmbeddedControlBlock(Args&&... args) {
        // 易错点: 用定位 new 在原始存储上构造对象；必须 alignas(T) 保证对齐。
        ::new (static_cast<void*>(storage)) T(std::forward<Args>(args)...);
    }

    T* ptr() noexcept { return std::launder(reinterpret_cast<T*>(storage)); }
    void destroy_object() noexcept override { ptr()->~T(); }
};

// ---------------- SharedPtr ----------------
template <class T>
class SharedPtr {
public:
    SharedPtr() noexcept = default;
    SharedPtr(std::nullptr_t) noexcept {}

    // 从裸指针接管（带默认删除器）
    template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    explicit SharedPtr(U* p) : ptr_(p) {
        if (p) cb_ = new PtrControlBlock<U, DefaultDelete<U>>(p, DefaultDelete<U>{});
    }

    // 带自定义删除器。易错点: 删除器类型 D 出现在这里，但【不】出现在 SharedPtr<T> 的类型里——
    //   控制块通过虚函数擦除了 D，这正是"类型擦除"：不同删除器的 SharedPtr 是同一类型。
    template <class U, class D,
              class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    SharedPtr(U* p, D d) : ptr_(p) {
        if (p) cb_ = new PtrControlBlock<U, D>(p, std::move(d));
    }

    SharedPtr(const SharedPtr& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) {
        if (cb_) cb_->add_strong();
    }

    // 跨类型拷贝（U* → T*）
    template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    SharedPtr(const SharedPtr<U>& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) {
        if (cb_) cb_->add_strong();
    }

    SharedPtr(SharedPtr&& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) {
        o.cb_ = nullptr;
        o.ptr_ = nullptr;
    }

    ~SharedPtr() { if (cb_) cb_->release_strong(); }

    SharedPtr& operator=(const SharedPtr& o) noexcept {
        // 训练点: copy-and-swap。若先减自己的旧计数再增新计数，自赋值时可能把自己释放掉。
        //   构造临时（＋1）→ swap → 临时析构（释放旧的）—— 顺序天然安全。
        SharedPtr(o).swap(*this);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& o) noexcept {
        SharedPtr(std::move(o)).swap(*this);
        return *this;
    }

    void reset() noexcept { SharedPtr().swap(*this); }

    T* get() const noexcept { return ptr_; }
    T& operator*()  const { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    // 易错点: 这是"近似"值——多线程下其它线程可能同时增减，读到的是快照。
    long use_count() const noexcept {
        return cb_ ? cb_->strong.load(std::memory_order_relaxed) : 0;
    }

    void swap(SharedPtr& o) noexcept {
        std::swap(cb_, o.cb_);
        std::swap(ptr_, o.ptr_);
    }

private:
    // 内部构造：接管一个已存在的控制块（对端已持有强引用）。供 WeakPtr::lock / MakeShared 用。
    SharedPtr(ControlBlockBase* cb, T* p) noexcept : cb_(cb), ptr_(p) {}

    ControlBlockBase* cb_  = nullptr;
    T*                ptr_ = nullptr;

    template <class U> friend class SharedPtr;
    template <class U> friend class WeakPtr;
    template <class U, class... A> friend SharedPtr<U> MakeShared(A&&...);
};

// ---------------- WeakPtr ----------------
template <class T>
class WeakPtr {
public:
    WeakPtr() noexcept = default;

    // 从 SharedPtr 构造：增加【弱】计数，不增加强计数 → 不阻止对象析构
    template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    WeakPtr(const SharedPtr<U>& sp) noexcept : cb_(sp.cb_), ptr_(sp.ptr_) {
        if (cb_) cb_->add_weak();
    }

    WeakPtr(const WeakPtr& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) { if (cb_) cb_->add_weak(); }
    template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    WeakPtr(const WeakPtr<U>& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) { if (cb_) cb_->add_weak(); }

    WeakPtr(WeakPtr&& o) noexcept : cb_(o.cb_), ptr_(o.ptr_) {
        o.cb_ = nullptr;
        o.ptr_ = nullptr;
    }

    ~WeakPtr() { if (cb_) cb_->release_weak(); }

    WeakPtr& operator=(const WeakPtr& o) noexcept { WeakPtr(o).swap(*this); return *this; }
    WeakPtr& operator=(WeakPtr&& o) noexcept { WeakPtr(std::move(o)).swap(*this); return *this; }

    // lock: 尝试提升成 SharedPtr。对象还活着才成功，否则返回空。
    SharedPtr<T> lock() const noexcept {
        if (!cb_) return {};
        // 训练点: 这是经典的"检查-然后-使用"竞态，必须用 CAS 循环：
        //   若先 load 再 add，两步之间对象可能已被另一个线程销毁。
        long s = cb_->strong.load(std::memory_order_acquire);
        while (s != 0) {
            if (cb_->strong.compare_exchange_weak(s, s + 1,
                                                  std::memory_order_acq_rel,
                                                  std::memory_order_acquire)) {
                return SharedPtr<T>(cb_, ptr_);   // 成功抢占一个强引用
            }
        }
        return {};
    }

    bool expired() const noexcept {
        return !cb_ || cb_->strong.load(std::memory_order_acquire) == 0;
    }

    long use_count() const noexcept {
        return cb_ ? cb_->strong.load(std::memory_order_relaxed) : 0;
    }

    void reset() noexcept { WeakPtr().swap(*this); }
    void swap(WeakPtr& o) noexcept {
        std::swap(cb_, o.cb_);
        std::swap(ptr_, o.ptr_);
    }

private:
    ControlBlockBase* cb_  = nullptr;
    T*                ptr_ = nullptr;

    template <class U> friend class WeakPtr;
    template <class U> friend class SharedPtr;
};

// ---------------- MakeShared ----------------
template <class T, class... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    // 一次分配：块里内嵌 T 的存储。对比 SharedPtr<T>(new T(...)) 需要两次分配。
    auto* cb = new EmbeddedControlBlock<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(cb, cb->ptr());
}

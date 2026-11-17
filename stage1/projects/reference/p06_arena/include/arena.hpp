#pragma once
// ArenaResource / MonotonicAllocator —— 线性（bump）分配器（项目 6）
// ============================================================================
// 训练点：分配器模型、对齐计算、整数溢出、零运行期 malloc。
//
// 单调分配的核心思想：从一大块预备内存里"撞指针"式地向上分配，只增不减，
// 单次分配 O(1) 且【永远不 free】。整块内存由 reset() 一次性回收。
// 这正是机器人控制环里"预先备好内存、运行期不 malloc"的做法。
//
// 对齐要点：底层缓冲按 kMaxAlign 对齐分配，之后把游标按请求的 align（2 的幂）向上取整，
// 结果地址一定是 align 的倍数（因为 base 是 kMaxAlign 的倍数，且 align | kMaxAlign）。
// ============================================================================

#include <cstddef>
#include <memory>
#include <memory_resource>
#include <new>
#include <stdexcept>
#include <type_traits>

class ArenaResource {
public:
    // 底层缓冲保证的对齐：至少 64，覆盖常见 SIMD/缓存行对齐需求。
    static constexpr std::size_t kMaxAlign =
        alignof(std::max_align_t) > 64 ? alignof(std::max_align_t) : 64;

    explicit ArenaResource(std::size_t capacity) : capacity_(capacity) {
        // 用【对齐版】operator new，保证 base_ 是 kMaxAlign 对齐的；否则超对齐类型会错位。
        base_ = static_cast<unsigned char*>(::operator new(capacity, std::align_val_t{kMaxAlign}));
    }
    ArenaResource(const ArenaResource&)            = delete;
    ArenaResource& operator=(const ArenaResource&) = delete;
    ~ArenaResource() { ::operator delete(base_, std::align_val_t{kMaxAlign}); }

    // 分配 bytes 字节、按 align 对齐。只前进游标，不记录、不可单独释放。
    void* allocate(std::size_t bytes, std::size_t align = alignof(std::max_align_t)) {
        if (align == 0 || (align & (align - 1)) != 0)
            throw std::invalid_argument("ArenaResource: align 必须是 2 的幂");
        if (align > kMaxAlign)
            throw std::bad_alloc();              // 超出底层缓冲能保证的对齐

        // 易错点: 对齐是"加上 align-1 再抹掉低位"，这一步本身可能【加法溢出】。
        std::size_t aligned = offset_ + (align - 1);
        if (aligned < offset_) throw std::bad_alloc();      // 环绕 → 溢出
        aligned &= ~(align - 1);

        // 易错点: `offset_ + bytes > capacity_` 会在加法上溢出。改成减法判越界更安全。
        if (aligned > capacity_ || bytes > capacity_ - aligned) throw std::bad_alloc();

        void* p = base_ + aligned;
        offset_ = aligned + bytes;
        return p;
    }

    void reset() noexcept { offset_ = 0; }       // 一次性回收（下次分配从头开始复用）

    std::size_t capacity()  const noexcept { return capacity_; }
    std::size_t used()      const noexcept { return offset_; }
    std::size_t remaining() const noexcept { return capacity_ - offset_; }

private:
    unsigned char* base_ = nullptr;
    std::size_t capacity_ = 0;
    std::size_t offset_ = 0;
};

// ---------------------------------------------------------------- 适配容器
// 单调分配器：把容器对"n 个 T"的请求转成 arena 的一次 bump 分配。
// 满足 Allocator 要求（value_type / allocate / deallocate / rebind / ==）。
template <class T>
class MonotonicAllocator {
public:
    using value_type                             = T;
    using size_type                              = std::size_t;
    using difference_type                        = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_swap            = std::false_type;
    using is_always_equal                        = std::false_type;

    template <class U> struct rebind { using other = MonotonicAllocator<U>; };

    MonotonicAllocator() noexcept : arena_(nullptr) {}
    explicit MonotonicAllocator(ArenaResource* a) noexcept : arena_(a) {}
    template <class U>
    MonotonicAllocator(const MonotonicAllocator<U>& o) noexcept : arena_(o.arena()) {}

    T* allocate(std::size_t n) {
        if (n > max_size()) throw std::bad_alloc();          // 溢出检查
        if (arena_ == nullptr) return std::allocator<T>{}.allocate(n);   // 退化：无 arena
        void* p = arena_->allocate(n * sizeof(T), alignof(T));
        return static_cast<T*>(p);
    }
    // 易错点: 单调分配器不做单次回收——deallocate 是空操作，内存等到 reset() 才整体归还。
    void deallocate(T* p, std::size_t n) noexcept {
        if (arena_ == nullptr && p != nullptr) std::allocator<T>{}.deallocate(p, n);
    }

    std::size_t max_size() const noexcept { return static_cast<std::size_t>(-1) / sizeof(T); }
    ArenaResource* arena() const noexcept { return arena_; }

    // 相等 = 用同一个 arena。容器据此判断能否 O(1) 偷缓冲/交换。
    friend bool operator==(const MonotonicAllocator& a, const MonotonicAllocator& b) noexcept { return a.arena_ == b.arena_; }
    friend bool operator!=(const MonotonicAllocator& a, const MonotonicAllocator& b) noexcept { return a.arena_ != b.arena_; }

private:
    ArenaResource* arena_;
};

// 选做：把 Arena 接成 std::pmr::memory_resource，可与 std::pmr 容器联用。
class ArenaMemoryResource : public std::pmr::memory_resource {
public:
    explicit ArenaMemoryResource(ArenaResource* a) noexcept : arena_(a) {}
private:
    void* do_allocate(std::size_t bytes, std::size_t alignment) override {
        return arena_->allocate(bytes, alignment);
    }
    void do_deallocate(void*, std::size_t, std::size_t) override {}   // 单调：no-op
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {
        return this == &other;
    }
    ArenaResource* arena_;
};

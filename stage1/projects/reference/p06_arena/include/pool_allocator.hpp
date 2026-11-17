#pragma once
// Pool / PoolAllocator<T> —— 定长块对象池 + 空闲链表（项目 6）
// ============================================================================
// 训练点：定长块、侵入式空闲链表、块对齐、溢出的块数计算、与容器（List）配合。
//
// 对象池解决的是"频繁 new/delete 同一种小对象"的开销：一次性切好一串等大块，
// 释放时把块头当作 next 指针塞回空闲链表 —— O(1) 分配/释放、无碎片、零运行期 malloc。
//
// 易错点:
//   1) 块大小必须 >= sizeof(void*)，否则存不下这条侵入式 next 指针。
//   2) 块步长必须是 block_align 的倍数，否则第 i 块的地址会偏离对齐。
//   3) 切 chunk 时的 blocks * block_size 可能【整数溢出】，要先检查。
// ============================================================================

#include <cstddef>
#include <memory>
#include <new>
#include <stdexcept>

#include "arena.hpp"   // 可选：从 ArenaResource 取 chunk，实现零运行期 malloc

class Pool {
public:
    // 不自带后备存储：chunk 从系统 operator new 取。
    explicit Pool(std::size_t block_size, std::size_t block_align = alignof(std::max_align_t))
        : Pool(block_size, block_align, nullptr, 64) {}

    // 从 arena 取 chunk（arena 非空时全程零 operator new）。
    Pool(std::size_t block_size, std::size_t block_align, ArenaResource* arena,
         std::size_t blocks_per_chunk = 64)
        : arena_(arena), blocks_per_chunk_(blocks_per_chunk ? blocks_per_chunk : 1) {
        if (block_align == 0 || (block_align & (block_align - 1)) != 0)
            throw std::invalid_argument("Pool: block_align 必须是 2 的幂");
        // 易错点 1：块至少要能塞下空闲链表指针。
        std::size_t bs = block_size < sizeof(void*) ? sizeof(void*) : block_size;
        // 易错点 2：块步长向上取整到 block_align 的倍数，保证每块都对齐。
        block_align_ = block_align;
        block_size_  = (bs + block_align - 1) & ~(block_align - 1);
    }

    Pool(const Pool&)            = delete;
    Pool& operator=(const Pool&) = delete;

    ~Pool() {
        for (Chunk* c = chunks_; c != nullptr;) {
            Chunk* next = c->next;
            // arena 版由 arena 统一回收；系统版要用【对齐版】delete 配对。
            if (arena_ == nullptr) ::operator delete(c->base, std::align_val_t{block_align_});
            c = next;
        }
    }

    // 取一块；空闲链表为空则先开一个新 chunk。依旧失败（如 arena 满）→ 抛 bad_alloc。
    void* allocate() {
        if (free_ == nullptr) grow();
        void* blk = free_;
        free_ = *static_cast<void**>(blk);      // 头 8 字节存着下一条空闲块
        ++live_;
        return blk;
    }

    // 归还一块：把块头改写成指向当前空闲链表，O(1)。
    void deallocate(void* p) noexcept {
        if (p == nullptr) return;
        *static_cast<void**>(p) = free_;
        free_ = p;
        --live_;
    }

    std::size_t block_size()  const noexcept { return block_size_; }
    std::size_t block_align() const noexcept { return block_align_; }
    std::size_t live_blocks() const noexcept { return live_; }         // 当前在用的块数
    std::size_t system_bytes() const noexcept { return system_bytes_; } // 累计向系统/arena 要了多少字节

private:
    struct Chunk { Chunk* next; unsigned char* base; };

    void grow() {
        // 易错点 3：先算总量再判溢出。offset 用来把块区对齐，头部是 Chunk。
        const std::size_t head = (sizeof(Chunk) + block_align_ - 1) & ~(block_align_ - 1);
        if (block_size_ != 0 && blocks_per_chunk_ > (static_cast<std::size_t>(-1) - head) / block_size_)
            throw std::bad_alloc();
        const std::size_t total = head + block_size_ * blocks_per_chunk_;

        unsigned char* raw;
        // 易错点: 系统版必须用【对齐版】new，否则 operator new 只保证 max_align_t 对齐，
        //         块步长再对齐也没用——起始地址就已经偏了。
        if (arena_ != nullptr) raw = static_cast<unsigned char*>(arena_->allocate(total, block_align_));
        else                   raw = static_cast<unsigned char*>(::operator new(total, std::align_val_t{block_align_}));
        system_bytes_ += total;

        Chunk* c = reinterpret_cast<Chunk*>(raw);
        c->base = raw;
        c->next = chunks_;
        chunks_ = c;

        // 把 [raw+head, raw+total) 切成 blocks_per_chunk_ 个等大块，串成空闲链表。
        unsigned char* blocks = raw + head;
        for (std::size_t i = 0; i < blocks_per_chunk_; ++i) {
            void* blk = blocks + i * block_size_;
            *static_cast<void**>(blk) = free_;
            free_ = blk;
        }
    }

    ArenaResource* arena_ = nullptr;
    std::size_t block_size_ = 0;
    std::size_t block_align_ = 0;
    std::size_t blocks_per_chunk_ = 64;
    void*       free_ = nullptr;
    Chunk*      chunks_ = nullptr;
    std::size_t live_ = 0;
    std::size_t system_bytes_ = 0;
};

// ---------------------------------------------------------------- 适配容器
// 定长池分配器：把一个 Pool 暴露成满足 Allocator 要求的类型，供 List 使用。
// 只服务"单对象"分配（n == 1）——List 每次只申请一个节点。
template <class T>
class PoolAllocator {
public:
    using value_type                             = T;
    using size_type                              = std::size_t;
    using difference_type                        = std::ptrdiff_t;
    using propagate_on_container_move_assignment = std::false_type;
    using propagate_on_container_copy_assignment = std::false_type;
    using propagate_on_container_swap            = std::false_type;
    using is_always_equal                        = std::false_type;

    template <class U> struct rebind { using other = PoolAllocator<U>; };

    PoolAllocator() noexcept : pool_(nullptr) {}
    explicit PoolAllocator(Pool* p) noexcept : pool_(p) {}
    template <class U>
    PoolAllocator(const PoolAllocator<U>& o) noexcept : pool_(o.pool()) {}

    T* allocate(std::size_t n) {
        if (n == 0) return nullptr;
        if (pool_ == nullptr) throw std::bad_alloc();     // 未绑定池
        if (n > max_size()) throw std::bad_alloc();       // 溢出检查
        if (n != 1) throw std::bad_alloc();               // 定长池只发单块
        if (sizeof(T) > pool_->block_size()) throw std::bad_alloc();   // 块太小装不下 T
        return static_cast<T*>(pool_->allocate());
    }
    void deallocate(T* p, std::size_t) noexcept {
        if (p != nullptr) pool_->deallocate(p);
    }

    // 块大小能被多少个 T 装下（这里只支持 1，仅作上界用途）。
    std::size_t max_size() const noexcept {
        return pool_ != nullptr ? pool_->block_size() / sizeof(T)
                                : static_cast<std::size_t>(-1) / sizeof(T);
    }
    Pool* pool() const noexcept { return pool_; }

    // 相等 = 用同一个池。rebind 出来的 PoolAllocator<U> 会带着同一个池指针。
    friend bool operator==(const PoolAllocator& a, const PoolAllocator& b) noexcept { return a.pool_ == b.pool_; }
    friend bool operator!=(const PoolAllocator& a, const PoolAllocator& b) noexcept { return a.pool_ != b.pool_; }

private:
    Pool* pool_;
};

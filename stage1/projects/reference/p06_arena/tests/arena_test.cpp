// 项目 6 参考测试：Arena（bump）与 Pool（定长块 + 空闲链表）分配器
#include <gtest/gtest.h>

#include <cstdint>
#include <cstddef>
#include <list>
#include <memory>
#include <memory_resource>
#include <random>
#include <vector>

#include "common/new_counter.hpp"
#include "common/throw_on_n.hpp"
#include "arena.hpp"
#include "list.hpp"
#include "pool_allocator.hpp"
#include "vector.hpp"

namespace {
struct alignas(64) Over64 { unsigned char data[64]; };
}

// ============================ Arena：对齐 ============================
TEST(Arena, AlignsToMaxAlignAndOverAligned) {
    ArenaResource arena(1 << 16);

    void* a = arena.allocate(8, alignof(std::max_align_t));
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(a) % alignof(std::max_align_t), 0u);

    // 训练点：超对齐（alignas(64)）也必须成立——底层缓冲按 kMaxAlign 对齐，
    //         游标再按 64 向上取整，二者相加仍是 64 的倍数。
    for (int i = 0; i < 8; ++i) {
        void* p = arena.allocate(1, alignof(Over64));
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % alignof(Over64), 0u);
    }

    auto* typed = static_cast<Over64*>(arena.allocate(sizeof(Over64), alignof(Over64)));
    typed->data[0] = 7;
    EXPECT_EQ(typed->data[0], 7);

    EXPECT_THROW(arena.allocate(8, 3), std::invalid_argument);   // 非 2 的幂
    EXPECT_THROW(arena.allocate(8, 4096), std::bad_alloc);       // 超过底层保证的对齐
}

// ============================ Arena：单调 + reset ============================
TEST(Arena, BumpIsMonotonicAndResetReuses) {
    ArenaResource arena(1 << 12);
    void* first = arena.allocate(16);
    void* second = arena.allocate(16);
    EXPECT_GT(reinterpret_cast<std::uintptr_t>(second), reinterpret_cast<std::uintptr_t>(first));
    const std::size_t used = arena.used();
    EXPECT_GE(used, 32u);

    arena.reset();                               // 一次性回收：游标归零，下次从头复用
    EXPECT_EQ(arena.used(), 0u);
    EXPECT_EQ(arena.remaining(), arena.capacity());
    void* again = arena.allocate(16);
    EXPECT_EQ(again, first);                     // 复用同一块地址
}

// ============================ Arena：溢出 / 越界 ============================
TEST(Arena, OverflowAndOverflowSafeAllocation) {
    ArenaResource arena(1024);
    // 请求远超容量：实现用【减法判越界】，不做会环绕的加法，故安全地抛 bad_alloc。
    EXPECT_THROW(arena.allocate(static_cast<std::size_t>(-1)), std::bad_alloc);
    EXPECT_THROW(arena.allocate(2048), std::bad_alloc);
    EXPECT_LE(arena.used(), arena.capacity());   // 失败的分配没有推进游标
}

TEST(MonotonicAllocator, LargeRequestThrowsNotOverflows) {
    ArenaResource arena(1024);
    MonotonicAllocator<int> alloc(&arena);
    // n > max_size() → 先做溢出检查，绝不进入 n*sizeof(T) 的环绕乘法。
    EXPECT_THROW(alloc.allocate(static_cast<std::size_t>(-1)), std::bad_alloc);
    EXPECT_EQ(alloc.arena(), &arena);
}

// ============================ Arena 接入容器 ============================
TEST(Arena, VectorWithMonotonicAllocatorDifferential) {
    ArenaResource arena(1 << 22);
    using V = Vector<int, MonotonicAllocator<int>>;
    MonotonicAllocator<int> va(&arena);
    V a(va);
    std::vector<int> b;

    std::mt19937 rng(606);
    auto sync = [&] {
        ASSERT_EQ(a.size(), b.size());
        for (std::size_t i = 0; i < b.size(); ++i) ASSERT_EQ(a[i], b[i]);
    };
    for (int step = 0; step < 5000; ++step) {
        switch (rng() % 6) {
            case 0: { int v = static_cast<int>(rng() % 100); a.push_back(v); b.push_back(v); break; }
            case 1: if (!b.empty()) { a.pop_back(); b.pop_back(); } break;
            case 2: { std::size_t n = rng() % 32; a.resize(n); b.resize(n); break; }
            case 3: if (!b.empty()) {
                std::size_t p = rng() % b.size();
                a.erase(a.begin() + static_cast<std::ptrdiff_t>(p));
                b.erase(b.begin() + static_cast<std::ptrdiff_t>(p));
            } break;
            case 4: { int v = static_cast<int>(rng() % 100); a.insert(a.begin(), v); b.insert(b.begin(), v); break; }
            case 5: if (b.size() > 100) { a.clear(); b.clear(); } break;
        }
        sync();
    }
}

TEST(Arena, ListWithMonotonicAllocatorIsZeroMallocAtRuntime) {
    ArenaResource arena(1 << 20);                // 缓冲在【构造期】一次性取得
    using L = List<int, MonotonicAllocator<int>>;

    NewCounter::reset();                         // 清零后立刻测量运行期
    {
        MonotonicAllocator<int> va(&arena);
        L l(va);
        for (int i = 0; i < 1000; ++i) l.push_back(i);
        // 训练点：节点全部从 arena bump 出来，运行期【零次】operator new。
        EXPECT_EQ(NewCounter::allocs(), 0L);
        EXPECT_EQ(l.size(), 1000u);
        EXPECT_EQ(l.back(), 999);
    }
    EXPECT_EQ(NewCounter::allocs(), 0L);
}

// ============================ Pool：复用 / 块对齐 ============================
TEST(Pool, ReusesFreedBlocksWithoutNewChunk) {
    ArenaResource arena(1 << 16);
    Pool pool(64, alignof(std::max_align_t), &arena, 8);

    void* a = pool.allocate();
    void* b = pool.allocate();
    const std::size_t sys = pool.system_bytes();
    EXPECT_EQ(pool.live_blocks(), 2u);

    pool.deallocate(a);                          // 归还到空闲链表
    pool.deallocate(b);
    EXPECT_EQ(pool.live_blocks(), 0u);

    void* c = pool.allocate();                   // 复用，不再开 chunk
    void* d = pool.allocate();
    EXPECT_EQ(pool.system_bytes(), sys);
    EXPECT_EQ(pool.live_blocks(), 2u);
    EXPECT_TRUE(c == a || c == b);
    EXPECT_TRUE(d == a || d == b);
}

TEST(Pool, BlockSizeRoundsUpAndBlocksAreAligned) {
    Pool pool(200, 64);                          // 块长 200 → 向上取整到 64 的倍数 = 256
    EXPECT_EQ(pool.block_size(), 256u);
    EXPECT_EQ(pool.block_align(), 64u);
    for (int i = 0; i < 8; ++i) {
        void* p = pool.allocate();
        EXPECT_EQ(reinterpret_cast<std::uintptr_t>(p) % 64, 0u);
    }
}

TEST(Pool, ExhaustionThrowsBadAlloc) {
    ArenaResource small(20000);
    Pool pool(1024, alignof(std::max_align_t), &small, 4);
    // 池满行为【显式】：arena 供不上新 chunk 时，allocate 抛 bad_alloc，绝不返回野指针。
    std::size_t n = 0;
    bool threw = false;
    try {
        for (; n < 1000000; ++n) (void)pool.allocate();
    } catch (const std::bad_alloc&) {
        threw = true;
    }
    EXPECT_TRUE(threw);
    EXPECT_GT(n, 0u);
}

TEST(PoolAllocator, RejectsMultiObjectAndUnbound) {
    ArenaResource arena(1 << 16);
    Pool pool(128, alignof(std::max_align_t), &arena, 16);
    PoolAllocator<int> alloc(&pool);
    EXPECT_THROW(alloc.allocate(2), std::bad_alloc);      // 定长池只发单块
    int* single = alloc.allocate(1);                       // 单块可用
    ASSERT_NE(single, nullptr);
    alloc.deallocate(single, 1);

    PoolAllocator<int> unbound;                           // 未绑定池
    EXPECT_THROW(unbound.allocate(1), std::bad_alloc);
}

// ============================ Pool 接入 List ============================
TEST(Pool, ListWithPoolAllocatorDifferential) {
    ArenaResource arena(1 << 22);
    Pool pool(128, alignof(std::max_align_t), &arena, 256);
    PoolAllocator<int> pa(&pool);
    List<int, PoolAllocator<int>> a(pa);
    std::list<int> b;

    std::mt19937 rng(6060);
    auto sync = [&] {
        ASSERT_EQ(a.size(), b.size());
        auto ia = a.begin();
        for (int v : b) { ASSERT_EQ(*ia, v); ++ia; }
        ASSERT_EQ(ia, a.end());
    };
    for (int step = 0; step < 8000; ++step) {
        switch (rng() % 6) {
            case 0: { int v = static_cast<int>(rng() % 100); a.push_back(v);  b.push_back(v);  break; }
            case 1: { int v = static_cast<int>(rng() % 100); a.push_front(v); b.push_front(v); break; }
            case 2: if (!b.empty()) { a.pop_front(); b.pop_front(); } break;
            case 3: if (!b.empty()) { a.pop_back();  b.pop_back();  } break;
            case 4: { int v = static_cast<int>(rng() % 100); a.push_back(v); b.push_back(v); break; }
            case 5: if (b.size() > 400) { a.clear(); b.clear(); } break;
        }
        sync();
    }
    // 复用充分：池向 arena 要的字节远小于"每次都新分配"的累积量。
    EXPECT_GT(pool.live_blocks(), 0u);
}

TEST(Pool, ListWithPoolAllocatorIsZeroMallocAtRuntime) {
    ArenaResource arena(1 << 20);
    Pool pool(128, alignof(std::max_align_t), &arena, 64);
    using L = List<int, PoolAllocator<int>>;

    NewCounter::reset();
    {
        PoolAllocator<int> pa(&pool);
        L l(pa);
        for (int i = 0; i < 500; ++i) l.push_back(i);
        for (int i = 0; i < 250; ++i) l.pop_front();     // 释放的块回到空闲链表
        for (int i = 0; i < 250; ++i) l.push_back(i);    // 复用，不开新 chunk
        EXPECT_EQ(NewCounter::allocs(), 0L);             // 运行期零 malloc
        EXPECT_EQ(l.size(), 500u);
    }
    EXPECT_EQ(NewCounter::allocs(), 0L);
}

// ============================ 选做：pmr ============================
TEST(Arena, PmrVectorOverArena) {
    ArenaResource arena(1 << 16);
    ArenaMemoryResource res(&arena);
    std::pmr::vector<int> v(&res);
    for (int i = 0; i < 100; ++i) v.push_back(i);
    ASSERT_EQ(v.size(), 100u);
    EXPECT_EQ(v[99], 99);
    EXPECT_GT(arena.used(), 0u);                 // 内存确实来自 arena
}

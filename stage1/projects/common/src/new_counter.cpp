// NewCounter 的计数存储 + 全局 operator new/delete 替换
// 说明：替换函数不能声明为 inline，故实现放在 .cpp，编译进 common 静态库。
// 注意：必须提供"完整的一套"替换形式（throwing / nothrow / aligned / 各 delete，
//       以及 nothrow 的 placement delete），否则某些路径（如 std::get_temporary_buffer
//       用的 nothrow new）会走 ASan/标准库的分配，却被我们的 delete 释放 —— ASan 会报
//       alloc-dealloc-mismatch。这里统一走 malloc/free，保证分配与释放配对一致。

#include "common/new_counter.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace {
std::atomic<long> g_allocs{0};
std::atomic<long> g_bytes{0};

void* raw_alloc(std::size_t sz) noexcept {
    if (sz == 0) sz = 1;
    return std::malloc(sz);
}

void* raw_alloc_aligned(std::size_t sz, std::size_t al) noexcept {
    if (sz == 0) sz = 1;
    // aligned_alloc 要求 size 是 alignment 的整数倍，向上取整
    std::size_t rounded = (sz + al - 1) / al * al;
    return std::aligned_alloc(al, rounded);
}
}  // namespace

void NewCounter::record(std::size_t sz) noexcept {
    g_allocs.fetch_add(1, std::memory_order_relaxed);
    g_bytes.fetch_add(static_cast<long>(sz), std::memory_order_relaxed);
}

void NewCounter::reset() noexcept {
    g_allocs.store(0, std::memory_order_relaxed);
    g_bytes.store(0, std::memory_order_relaxed);
}

long NewCounter::allocs() noexcept { return g_allocs.load(std::memory_order_relaxed); }
long NewCounter::bytes()  noexcept { return g_bytes.load(std::memory_order_relaxed); }

// ---------- throwing new ----------
void* operator new(std::size_t sz) {
    NewCounter::record(sz);
    if (void* p = raw_alloc(sz)) return p;
    throw std::bad_alloc();
}

void* operator new[](std::size_t sz) { return ::operator new(sz); }

void* operator new(std::size_t sz, std::align_val_t al) {
    NewCounter::record(sz);
    if (void* p = raw_alloc_aligned(sz, static_cast<std::size_t>(al))) return p;
    throw std::bad_alloc();
}

void* operator new[](std::size_t sz, std::align_val_t al) { return ::operator new(sz, al); }

// ---------- nothrow new ----------
void* operator new(std::size_t sz, const std::nothrow_t&) noexcept {
    NewCounter::record(sz);
    return raw_alloc(sz);
}

void* operator new[](std::size_t sz, const std::nothrow_t&) noexcept {
    return ::operator new(sz, std::nothrow);
}

void* operator new(std::size_t sz, std::align_val_t al, const std::nothrow_t&) noexcept {
    NewCounter::record(sz);
    return raw_alloc_aligned(sz, static_cast<std::size_t>(al));
}

void* operator new[](std::size_t sz, std::align_val_t al, const std::nothrow_t&) noexcept {
    return ::operator new(sz, al, std::nothrow);
}

// ---------- delete ----------
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }

// ---------- nothrow placement delete（构造函数抛异常时配对调用）----------
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t, const std::nothrow_t&) noexcept { std::free(p); }

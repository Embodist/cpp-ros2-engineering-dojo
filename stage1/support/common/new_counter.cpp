#include "common/new_counter.hpp"

#include <atomic>
#include <cstdlib>
#include <new>

namespace dojo::test {
namespace {
std::atomic<long> g_allocs{0};
std::atomic<long> g_bytes{0};
}  // namespace

void NewCounter::reset() noexcept {
    g_allocs.store(0, std::memory_order_relaxed);
    g_bytes.store(0, std::memory_order_relaxed);
}

long NewCounter::allocs() noexcept {
    return g_allocs.load(std::memory_order_relaxed);
}

long NewCounter::bytes() noexcept {
    return g_bytes.load(std::memory_order_relaxed);
}

void NewCounter::record(std::size_t size) noexcept {
    g_allocs.fetch_add(1, std::memory_order_relaxed);
    g_bytes.fetch_add(static_cast<long>(size), std::memory_order_relaxed);
}

}  // namespace dojo::test

void* operator new(std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    dojo::test::NewCounter::record(size);
    return p;
}

void* operator new[](std::size_t size) {
    void* p = std::malloc(size);
    if (!p) throw std::bad_alloc();
    dojo::test::NewCounter::record(size);
    return p;
}

// 易错点：nothrow 版本也必须替换。否则 ASan 会拦截 nothrow new，而我们的 delete
// 仍然走 free，触发 alloc-dealloc-mismatch（operator new vs free）。
void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    void* p = std::malloc(size);
    if (p) dojo::test::NewCounter::record(size);
    return p;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    void* p = std::malloc(size);
    if (p) dojo::test::NewCounter::record(size);
    return p;
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, const std::nothrow_t&) noexcept { std::free(p); }
void operator delete[](void* p, const std::nothrow_t&) noexcept { std::free(p); }

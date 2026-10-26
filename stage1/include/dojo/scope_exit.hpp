#pragma once
#include <functional>
#include <utility>

namespace dojo {
// TODO: 不要让移动后的源对象重复执行 callback；保持析构 noexcept。
class ScopeExit {
public:
    explicit ScopeExit(std::function<void()> fn) : fn_(std::move(fn)) {}
    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit(ScopeExit&& other) noexcept;           // TODO
    ScopeExit& operator=(ScopeExit&& other) noexcept; // TODO: 原对象尚持有清理任务时怎么处理？
    ~ScopeExit() noexcept;                          // TODO
    void release() noexcept;                        // TODO
private:
    std::function<void()> fn_;
    bool active_ = true;
};
// TODO: 在此文件中定义上述方法（header-only 用 inline）。
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline ScopeExit::ScopeExit(ScopeExit&& other) noexcept : fn_(std::move(other.fn_)), active_(false) { (void)other; /* TODO */ }
inline ScopeExit& ScopeExit::operator=(ScopeExit&& other) noexcept { (void)other; return *this; /* TODO */ }
inline ScopeExit::~ScopeExit() noexcept { /* TODO */ }
inline void ScopeExit::release() noexcept { /* TODO */ }

}

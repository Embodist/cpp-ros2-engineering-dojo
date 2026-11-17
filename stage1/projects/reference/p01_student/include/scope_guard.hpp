#pragma once
// ScopeGuard —— 析构时执行回调的作用域守卫（可 dismiss）
// ============================================================================
// 它解决什么问题：资源清理/回滚代码要在"正常返回"和"抛异常"两条路径上都执行。
// 手写 try/catch 又丑又容易漏。RAII 守卫让析构函数统一兜底。
//
//   { ScopeGuard g([&]{ std::filesystem::remove(tmp); });  // 失败时删临时文件
//     write_to(tmp);                                       // 抛异常也会执行 g
//     std::filesystem::rename(tmp, dst);
//     g.dismiss();                                         // 成功则撤销清理
//   }
//
// 参考: Core Guidelines E.6（用 RAII 表达"必须成对的操作"）；E.19（优先 RAII 而非 finally）。
// 易错点: 守卫必须不可拷贝——可拷贝会有两个对象析构时各执行一次回调。
// ============================================================================

#include <functional>
#include <utility>

class ScopeGuard {
public:
    explicit ScopeGuard(std::function<void()> f) : f_(std::move(f)) {}

    ~ScopeGuard() {
        if (active_ && f_) f_();
    }

    // 撤销：不再执行回调（用于"成功路径"）
    void dismiss() noexcept { active_ = false; }

    // 只允许移动（转移守卫的所有权）
    ScopeGuard(ScopeGuard&& o) noexcept : f_(std::move(o.f_)), active_(o.active_) {
        o.active_ = false;
    }
    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    std::function<void()> f_;
    bool active_ = true;
};

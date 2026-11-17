#pragma once
// NoexceptToggle<B> —— 用模板参数控制"移动构造是否 noexcept"
// ============================================================================
// 它解决什么问题：验证容器扩容时 move_if_noexcept 的取舍。
//   NoexceptToggle<true>  移动是 noexcept  → 扩容走【搬移】
//   NoexceptToggle<false> 移动非 noexcept 且类型可拷贝 → 扩容走【拷贝】
// 两个版本都测一遍，就能证明你的 Vector 真的按 noexcept 做了分支，而不是无脑搬移。
// ============================================================================

template <bool NoexceptMove>
struct NoexceptToggle {
    int value = 0;

    NoexceptToggle() = default;
    explicit NoexceptToggle(int v) : value(v) {}

    // ======================= 本文件最核心的易错点 =======================
    // 一旦你【声明】了移动构造或移动赋值，编译器就【不会】再隐式生成拷贝构造/拷贝赋值
    // （它们会被定义为 deleted）。而 move_if_noexcept 需要"另一条路"——类型必须**可拷贝**，
    // 才会在移动不安全时改用拷贝。所以这里必须【显式】把拷贝 = default 补回来。
    // 新手常犯：只写移动、以为拷贝还在，结果 std::vector 相关代码编译报"无法拷贝"。
    // 参考: C++ 规则 [class.copy.ctor]/6；Core Guidelines C.21（"五法则"）。
    NoexceptToggle(const NoexceptToggle&) = default;
    NoexceptToggle& operator=(const NoexceptToggle&) = default;
    // ====================================================================

    // 取舍: 移动构造的 noexcept 由模板参数决定（noexcept(NoexceptMove) 是编译期常量表达式）。
    NoexceptToggle(NoexceptToggle&& o) noexcept(NoexceptMove) : value(o.value) { o.value = -1; }
    NoexceptToggle& operator=(NoexceptToggle&& o) noexcept(NoexceptMove) {
        value = o.value; o.value = -1; return *this;
    }
};

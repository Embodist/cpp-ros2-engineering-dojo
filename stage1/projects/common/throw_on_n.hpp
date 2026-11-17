#pragma once
// ThrowOnN —— 在第 N 次构造/拷贝时抛异常，用来扫描异常安全的所有抛出点
// ============================================================================
// 它解决什么问题：写容器（Vector/List/String…）时要验证"强异常保证"——中途抛异常
// 时原对象必须不变且不泄漏。用 ThrowOnN 把"第 N 次构造"变成抛点，就能逐个抛出点扫一遍。
//
// 用法：
//   ThrowOnN::arm(3);   // 从此刻起，第 3 次构造/拷贝构造/移动构造时抛 ThrowOnNError
//   ...被扫代码...
//   ThrowOnN::disarm();
// ============================================================================

#include <stdexcept>

// 为什么: 用专门的异常类型，测试里 EXPECT_THROW(..., ThrowOnNError) 才能精确区分
//         "我们注入的异常"和"被测代码自己抛的异常"。
struct ThrowOnNError : std::runtime_error {
    explicit ThrowOnNError(const char* what) : std::runtime_error(what) {}
};

struct ThrowOnN {
    int value = 0;

    // -1 表示关闭；>0 表示还剩几次触发
    inline static int remaining = -1;

    static void arm(int n)  noexcept { remaining = n; }
    static void disarm()    noexcept { remaining = -1; }

    // 易错点: 默认构造函数必须也 tick()。曾经这里写成 `ThrowOnN() = default;`，
    //         结果"第 N 次**构造**"里的默认构造根本不计数，测试里 arm(2) 永远不触发。
    //         凡是会"产生一个新 ThrowOnN 对象"的路径都要 tick()：默认/值/拷贝/移动构造。
    ThrowOnN() { tick(); }
    explicit ThrowOnN(int v) : value(v) { tick(); }

    ThrowOnN(const ThrowOnN& o) : value(o.value) { tick(); }
    // 取舍: 移动构造标 noexcept(false)。因为移动也可能抛，才能覆盖"容器搬移中途失败"的抛出点。
    //       代价：这样的类型在 move_if_noexcept 下会被当作"不可安全移动"，容器扩容会改用拷贝。
    ThrowOnN(ThrowOnN&& o) noexcept(false) : value(o.value) { o.value = -1; tick(); }

    ThrowOnN& operator=(const ThrowOnN& o) { value = o.value; return *this; }
    ThrowOnN& operator=(ThrowOnN&& o) noexcept(false) { value = o.value; o.value = -1; return *this; }

private:
    static void tick() {
        // 语义：arm(3) → 第1次 tick 剩2、第2次剩1、第3次减到 0 即抛
        if (remaining > 0 && --remaining == 0)
            throw ThrowOnNError("ThrowOnN: injected failure");
    }
};

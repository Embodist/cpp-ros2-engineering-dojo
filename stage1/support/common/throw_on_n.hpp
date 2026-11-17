#pragma once
// ThrowOnN：在第 N 次构造/拷贝/移动时抛异常，用来扫描异常安全的所有抛出点。
// 用法：ThrowOnN::arm(3); ...被测代码...; ThrowOnN::disarm();

#include <stdexcept>

namespace dojo::test {

// 专用异常类型，测试里 EXPECT_THROW(..., ThrowOnNError) 才能区分注入异常和被测代码自己的异常。
struct ThrowOnNError : std::runtime_error {
    explicit ThrowOnNError(const char* what) : std::runtime_error(what) {}
};

struct ThrowOnN {
    int value = 0;

    // -1 表示关闭；>0 表示还剩几次触发。
    inline static int remaining = -1;

    static void arm(int n) noexcept { remaining = n; }
    static void disarm() noexcept { remaining = -1; }

    // 易错点：默认构造也必须 tick。把 ThrowOnN() 写成 = default 会让“第 N 次构造”少算一次。
    ThrowOnN() { tick(); }
    explicit ThrowOnN(int v) : value(v) { tick(); }
    ThrowOnN(const ThrowOnN& o) : value(o.value) { tick(); }

    // 取舍：移动构造 noexcept(false)，才能覆盖“容器搬移中途失败”的抛出点；
    //       代价是 move_if_noexcept 会把它当“不可安全移动”，容器扩容改用拷贝。
    ThrowOnN(ThrowOnN&& o) noexcept(false) : value(o.value) {
        o.value = -1;
        tick();
    }

    ThrowOnN& operator=(const ThrowOnN& o) { value = o.value; return *this; }
    ThrowOnN& operator=(ThrowOnN&& o) noexcept(false) {
        value = o.value;
        o.value = -1;
        return *this;
    }

private:
    static void tick() {
        if (remaining > 0 && --remaining == 0) {
            throw ThrowOnNError("ThrowOnN: injected failure");
        }
    }
};

}  // namespace dojo::test

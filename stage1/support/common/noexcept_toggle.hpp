#pragma once
// NoexceptToggle<B>：用模板参数控制移动构造是否 noexcept。
//   true  → 扩容走搬移
//   false → 移动可能抛且类型可拷贝 → 扩容退回拷贝
// 两个版本都测，才能证明容器真的按 noexcept 分支，而不是无脑搬移。

namespace dojo::test {

template <bool NoexceptMove>
struct NoexceptToggle {
    int value = 0;

    NoexceptToggle() = default;
    explicit NoexceptToggle(int v) : value(v) {}

    // 易错点：一旦声明了移动构造/赋值，编译器不再隐式生成拷贝构造/赋值。
    //         move_if_noexcept 需要“另一条路”——类型必须可拷贝，才能退回拷贝。
    NoexceptToggle(const NoexceptToggle&) = default;
    NoexceptToggle& operator=(const NoexceptToggle&) = default;

    NoexceptToggle(NoexceptToggle&& o) noexcept(NoexceptMove) : value(o.value) {
        o.value = -1;
    }
    NoexceptToggle& operator=(NoexceptToggle&& o) noexcept(NoexceptMove) {
        value = o.value;
        o.value = -1;
        return *this;
    }
};

}  // namespace dojo::test

#pragma once
// Tracer：统计六类特殊成员操作和存活对象数。
// 用途：用数字证明“零拷贝 / 走移动 / 构造数等于析构数”，而不是靠感觉。

#include <cstddef>

namespace dojo::test {

class Tracer {
public:
    // inline static：头文件被多个 TU 包含也只有一份定义。
    inline static long default_ctor = 0;
    inline static long value_ctor   = 0;
    inline static long copy_ctor    = 0;
    inline static long move_ctor    = 0;
    inline static long copy_assign  = 0;
    inline static long move_assign  = 0;
    inline static long dtor         = 0;
    // live 用有符号 long：多析构一次会变成负数，测试能看出来；用 size_t 会回绕成巨大正数。
    inline static long live         = 0;

    Tracer() { ++default_ctor; ++live; id_ = next_id_++; }
    explicit Tracer(int v) : value_(v) { ++value_ctor; ++live; id_ = next_id_++; }

    // 易错点：赋值不创建/销毁对象，所以 copy_assign/move_assign 不能改 live。
    Tracer(const Tracer& o) : value_(o.value_) { ++copy_ctor; ++live; id_ = next_id_++; }
    Tracer(Tracer&& o) noexcept : value_(o.value_) {
        o.value_ = -1;
        ++move_ctor;
        ++live;
        id_ = next_id_++;
    }
    Tracer& operator=(const Tracer& o) { value_ = o.value_; ++copy_assign; return *this; }
    Tracer& operator=(Tracer&& o) noexcept {
        value_ = o.value_;
        o.value_ = -1;
        ++move_assign;
        return *this;
    }

    ~Tracer() { ++dtor; --live; }

    int value() const noexcept { return value_; }
    long id() const noexcept { return id_; }

    static long total_ctor() noexcept { return default_ctor + value_ctor + copy_ctor + move_ctor; }
    static long total_copy() noexcept { return copy_ctor + copy_assign; }
    static long total_move() noexcept { return move_ctor + move_assign; }
    static bool balanced() noexcept { return live == 0; }

    // 约定：只在作用域内没有存活 Tracer 时 reset，否则 live 会被强行归零，析构后变负数。
    static void reset() noexcept {
        default_ctor = value_ctor = copy_ctor = move_ctor = 0;
        copy_assign = move_assign = dtor = 0;
        live = 0;
    }

private:
    inline static long next_id_ = 0;
    int value_ = 0;
    long id_ = 0;
};

}  // namespace dojo::test

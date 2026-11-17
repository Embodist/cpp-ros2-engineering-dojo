#pragma once
// Tracer —— 计数所有特殊成员操作的探针类型
// ============================================================================
// 它解决什么问题：C++ 里"到底考贝了几次 / 有没有多一次移动"肉眼看不出来。
// Tracer 把六类特殊成员操作都记成全局计数，测试里 reset() → 跑代码 → 读计数，
// 就能用数字证明"零拷贝""走的是移动不是拷贝""构造数 == 析构数"等命题。
//
// 用法：Tracer::reset(); ...被测代码...; EXPECT_EQ(Tracer::copy_ctor, 0);
// 每个实例在构造时分配一个全局唯一 id（拷贝/移动也会拿到新 id，表示"新实例"）。
// ============================================================================

#include <cstddef>

class Tracer {
public:
    // 易错点: 计数用 inline static（C++17）。若是普通 static 成员，就得在某个 .cpp 里
    //         再定义一次（否则链接报 undefined），而这个夹具是 header-only，会破坏布局。
    //         inline static 让"声明即定义"，头文件被多个 TU 包含也只存在一份。
    inline static long default_ctor = 0;
    inline static long value_ctor   = 0;
    inline static long copy_ctor    = 0;
    inline static long move_ctor    = 0;
    inline static long copy_assign  = 0;
    inline static long move_assign  = 0;
    inline static long dtor         = 0;
    // 易错点: live 用 long（有符号）。若用 size_t，一旦"多析构一次"会从 0 回绕成
    //         一个巨大正数，测试里 EXPECT_EQ(live, 0) 反而看不出来；有符号能暴露负值。
    inline static long live         = 0;

    Tracer()                       { ++default_ctor; ++live; id_ = next_id_++; }
    explicit Tracer(int v) : value_(v) { ++value_ctor;   ++live; id_ = next_id_++; }

    // 取舍: 拷贝/移动都让新实例拿新 id。这样"某元素到底被搬走了没"要靠 copy/move 计数判断，
    //       而不是靠 id 追踪。id 只表达"这是一个独立实例"。
    Tracer(const Tracer& o) : value_(o.value_) { ++copy_ctor; ++live; id_ = next_id_++; }
    // 为什么: 移动构造标 noexcept —— 容器扩容（如项目5 Vector）会用 move_if_noexcept 决定
    //         "搬移还是拷贝"，可 noexcept 移动的类型才会走高效的搬移路径。
    Tracer(Tracer&& o) noexcept : value_(o.value_) { o.value_ = -1; ++move_ctor; ++live; id_ = next_id_++; }

    // 易错点: 赋值（copy_assign / move_assign）**不能** 改 live —— 赋值没有创建/销毁对象，
    //         目标对象本来就活着。只有构造 +1、析构 -1。
    Tracer& operator=(const Tracer& o) { value_ = o.value_; ++copy_assign; return *this; }
    Tracer& operator=(Tracer&& o) noexcept { value_ = o.value_; o.value_ = -1; ++move_assign; return *this; }

    ~Tracer() { ++dtor; --live; }

    int  value() const noexcept { return value_; }
    long id()    const noexcept { return id_; }

    // 常用派生量，省得每次手算
    static long total_ctor()  noexcept { return default_ctor + value_ctor + copy_ctor + move_ctor; }
    static long total_copy()  noexcept { return copy_ctor + copy_assign; }
    static long total_move()  noexcept { return move_ctor + move_assign; }
    static bool balanced()    noexcept { return live == 0; }

    // 清零。易错点: 调用时若还有存活的 Tracer，live 会被强行置 0，之后析构会算成负数。
    //             约定：只在"作用域内无存活 Tracer"时 reset（测试开头通常满足）。
    static void reset() noexcept {
        default_ctor = value_ctor = copy_ctor = move_ctor = 0;
        copy_assign = move_assign = dtor = 0;
        live = 0;
    }

private:
    inline static long next_id_ = 0;
    int  value_ = 0;   // 移动后置 -1，用来演示"被搬走的对象不再持有原值"
    long id_    = 0;
};

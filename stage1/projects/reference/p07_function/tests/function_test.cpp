// 项目 7 参考测试：Function / MoveOnlyFunction（类型擦除 + SBO）
#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include "common/new_counter.hpp"
#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"
#include "function.hpp"

namespace {

struct Big { char data[64]{}; };

struct AddN {
    int n = 0;
    int operator()(int x) const { return x + n; }
};

// 持有 Tracer 的有状态 callable：用来数"存进 Function 后被拷贝/移动了几次"
struct TracerCallable {
    Tracer t;
    TracerCallable() = default;
    explicit TracerCallable(int v) : t(v) {}
    int operator()(int x) const { return x + t.value(); }
};

// 拷贝会抛的 callable：内部是 ThrowOnN，拷贝它时按 arm 计数抛异常
struct Thrower {
    ThrowOnN t;
    Thrower() = default;
    explicit Thrower(int v) : t(v) {}
    int operator()() const { return t.value; }
};

struct Widget {
    int base = 0;
    int add(int x) const { return base + x; }
};

int free_add(int x) { return x + 100; }

}  // namespace

// ============================ 类型性质 / noexcept ============================
TEST(Function, TypeTraitsAndNoexcept) {
    static_assert(std::is_nothrow_move_constructible_v<Function<int(int)>>);
    static_assert(std::is_nothrow_move_assignable_v<Function<int(int)>>);
    static_assert(std::is_nothrow_default_constructible_v<Function<int()>>);
    // MoveOnlyFunction 不能拷贝，只能移动
    static_assert(!std::is_copy_constructible_v<MoveOnlyFunction<int()>>);
    static_assert(std::is_move_constructible_v<MoveOnlyFunction<int()>>);
    SUCCEED();
}

// ============================ SBO：小零分配 / 大一次分配 ============================
TEST(Function, SmallCallableIsZeroHeapAlloc) {
    NewCounter::reset();
    Function<int(int)> f = [](int x) { return x * 2; };   // 无捕获 lambda：1 字节
    EXPECT_EQ(NewCounter::allocs(), 0L);                  // 存进内部缓冲，零堆分配
    EXPECT_EQ(f(21), 42);

    NewCounter::reset();
    Function<int(int)> g = &free_add;                     // 函数指针：8 字节，也在缓冲里
    EXPECT_EQ(NewCounter::allocs(), 0L);
    EXPECT_EQ(g(1), 101);
}

TEST(Function, LargeCallableAllocatesExactlyOnce) {
    NewCounter::reset();
    Function<int(int)> f = [b = Big{}](int x) { return x + b.data[0]; };  // 64 字节 > 缓冲
    EXPECT_EQ(NewCounter::allocs(), 1L);                  // 恰好一次堆分配
    EXPECT_EQ(f(5), 5);
}

// ============================ 有状态 callable 的拷贝/移动计数 ============================
TEST(Function, StatefulCallableCopyMoveCounts) {
    Tracer::reset();
    {
        Function<int(int)> f = TracerCallable(5);         // 临时对象移入 → 一次移动
        const long c0 = Tracer::copy_ctor;
        const long m0 = Tracer::move_ctor;

        Function<int(int)> g = f;                          // 深拷贝 Function → callable 拷贝一次
        EXPECT_EQ(Tracer::copy_ctor, c0 + 1);
        EXPECT_EQ(Tracer::move_ctor, m0);                  // 拷贝不产生额外移动

        EXPECT_EQ(f(1), 6);
        EXPECT_EQ(g(1), 6);

        Function<int(int)> h = std::move(g);               // 移动 → callable 移动一次
        EXPECT_EQ(Tracer::move_ctor, m0 + 1);
        EXPECT_EQ(h(1), 6);
    }
    EXPECT_TRUE(Tracer::balanced());                       // 构造数 == 析构数，无泄漏
}

// ============================ 空状态 / 移动后为空 ============================
TEST(Function, EmptyThrowsAndMovedFromBecomesEmpty) {
    Function<int()> f;
    EXPECT_FALSE(static_cast<bool>(f));
    EXPECT_THROW(f(), std::bad_function_call);

    Function<int()> g = [] { return 7; };
    EXPECT_TRUE(static_cast<bool>(g));

    Function<int()> h = std::move(g);
    EXPECT_FALSE(static_cast<bool>(g));                    // 源变空
    EXPECT_EQ(h(), 7);
    EXPECT_THROW(g(), std::bad_function_call);             // 空调用抛异常

    h.reset();
    EXPECT_FALSE(static_cast<bool>(h));
}

// ============================ swap：缓冲别名坑 ============================
TEST(Function, SwapOfTwoInlineCallables) {
    // 两个小 callable 都放在各自内部缓冲里。若 swap 天真地交换 obj_ 指针，
    // 两者会指向【对方的缓冲】→ 结果错乱甚至崩溃。
    Function<int()> a = [] { return 1; };
    Function<int()> b = [] { return 2; };
    swap(a, b);
    EXPECT_EQ(a(), 2);
    EXPECT_EQ(b(), 1);

    Function<int()> empty;
    swap(a, empty);
    EXPECT_FALSE(static_cast<bool>(a));
    EXPECT_EQ(empty(), 2);
}

// ============================ 拷贝抛异常不泄漏 ============================
TEST(Function, ThrowingCopyLeavesNoLeak) {
    Function<int()> f = Thrower(7);
    EXPECT_EQ(f(), 7);

    ThrowOnN::arm(1);                                      // 下一次拷贝构造即抛
    EXPECT_THROW(Function<int()> g(f), ThrowOnNError);
    ThrowOnN::disarm();

    EXPECT_EQ(f(), 7);                                     // 源不受影响
    // 若拷贝路径忘记在异常时释放堆内存，LeakSanitizer 会在进程退出时报出。
}

// ============================ MoveOnlyFunction ============================
TEST(Function, MoveOnlyStoresMoveOnlyCallable) {
    MoveOnlyFunction<int()> f = [p = std::make_unique<int>(41)] { return *p + 1; };
    EXPECT_EQ(f(), 42);

    MoveOnlyFunction<int()> g = std::move(f);
    EXPECT_EQ(g(), 42);
    EXPECT_FALSE(static_cast<bool>(f));

    // 移动-only 的 callable 也支持 swap（走移动，不拷贝）
    MoveOnlyFunction<int()> h = [] { return 0; };
    swap(g, h);
    EXPECT_EQ(h(), 42);
    EXPECT_EQ(g(), 0);
}

// ============================ std::invoke：函数指针 / 成员指针 ============================
TEST(Function, InvokesFunctionPointerAndMemberFunction) {
    Function<int(int)> f = &free_add;
    EXPECT_EQ(f(1), 101);

    Widget w{10};
    Function<int(const Widget&, int)> m = &Widget::add;    // 成员函数指针，绑定对象由调用方给
    EXPECT_EQ(m(w, 5), 15);
}

// ============================ 选做：target<T>() ============================
TEST(Function, TargetRecoversCallable) {
    Function<int(int)> f = &free_add;
    ASSERT_NE(f.target<int (*)(int)>(), nullptr);
    EXPECT_EQ((*f.target<int (*)(int)>())(1), 101);
    EXPECT_EQ(f.target<double (*)(int)>(), nullptr);       // 类型不符 → nullptr

    AddN add{100};
    Function<int(int)> g = add;
    EXPECT_NE(g.target<AddN>(), nullptr);
    EXPECT_EQ(g.target<AddN>()->n, 100);
}

// ============================ 引用捕获 vs 值捕获（生命周期说明） ============================
TEST(Function, CaptureByReferenceVsValue) {
    int outer = 1;

    Function<int()> by_ref = [&outer] { return outer; };   // 捕获引用：看到外部后续变化
    Function<int()> by_val = [outer] { return outer; };    // 捕获值：冻结当时的副本
    outer = 99;
    EXPECT_EQ(by_ref(), 99);
    EXPECT_EQ(by_val(), 1);

    // 易错点: by_ref 捕获的是 outer 的【引用】。若 outer 先于 Function 结束生命周期
    //         （例如外层变量出了作用域），这个 Function 就悬空——调用是 UB。
    //         所以把捕获引用的 lambda 存进 Function 长留，是典型的悬空来源。
}

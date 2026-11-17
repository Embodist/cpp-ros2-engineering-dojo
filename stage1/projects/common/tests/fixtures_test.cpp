// 夹具自身的测试：证明六个夹具按规格工作。
// 顺便演示"怎么用夹具证明命题"——后面每个项目都会重复这套手法。
#include <gtest/gtest.h>

#include <string>
#include <type_traits>

#include "common/leak_check.hpp"
#include "common/move_only.hpp"
#include "common/new_counter.hpp"
#include "common/noexcept_toggle.hpp"
#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"

TEST(TracerFixture, CountsEveryOperation) {
    Tracer::reset();
    {
        Tracer a;               // 默认构造
        Tracer b(7);            // 值构造
        Tracer c(b);            // 拷贝构造
        Tracer d(std::move(c)); // 移动构造
        a = b;                  // 拷贝赋值
        a = std::move(d);       // 移动赋值
    }
    EXPECT_EQ(Tracer::default_ctor, 1);
    EXPECT_EQ(Tracer::value_ctor, 1);
    EXPECT_EQ(Tracer::copy_ctor, 1);
    EXPECT_EQ(Tracer::move_ctor, 1);
    EXPECT_EQ(Tracer::copy_assign, 1);
    EXPECT_EQ(Tracer::move_assign, 1);
    EXPECT_EQ(Tracer::dtor, 4);   // 4 个对象 a/b/c/d 全部析构
    EXPECT_EQ(Tracer::live, 0);   // 且构造数 == 析构数
    EXPECT_TRUE(Tracer::balanced());
}

TEST(TracerFixture, UniqueIdPerInstance) {
    Tracer::reset();
    Tracer a, b;
    EXPECT_NE(a.id(), b.id());
    Tracer c(a);
    // 参考: 拷贝也是"新实例"，所以拿新 id。要判断"是否被搬走/拷贝"看的是 copy/move 计数。
    EXPECT_NE(c.id(), a.id());
}

TEST(LeakCheckFixture, PassesWhenNoLeak) {
    Tracer::reset();
    { LeakCheck lc; Tracer t; }  // t 在作用域内析构，退出时 live 回到 0 → 守卫通过
    SUCCEED();
}

TEST(ThrowOnNFixture, ThrowsOnNthConstruction) {
    ThrowOnN::arm(2);
    ThrowOnN a;                               // 第 1 次构造（默认构造也计数！）
    EXPECT_THROW(ThrowOnN b, ThrowOnNError);  // 第 2 次 → 抛
    ThrowOnN::disarm();
    ThrowOnN c;                               // 关闭后正常构造
    SUCCEED();
}

TEST(ThrowOnNFixture, ThrowsOnNthCopy) {
    ThrowOnN src(5);
    ThrowOnN::arm(1);
    EXPECT_THROW(ThrowOnN copy(src), ThrowOnNError);  // 第 1 次拷贝构造即抛
    ThrowOnN::disarm();
}

TEST(MoveOnlyFixture, IsMovableNotCopyable) {
    // 为什么要在编译期断言: 这些性质若被破坏，应该在编译时报错，而不是运行期才知道。
    static_assert(!std::is_copy_constructible_v<MoveOnly>);
    static_assert(!std::is_copy_assignable_v<MoveOnly>);
    static_assert(std::is_move_constructible_v<MoveOnly>);

    MoveOnly a(3);
    MoveOnly b(std::move(a));
    EXPECT_EQ(b.value(), 3);
}

TEST(NoexceptToggleFixture, NoexceptReflectsTemplateParam) {
    // 这两个断言直接验证"模板参数真的控制了 noexcept"——后面测 Vector 扩容分支靠它。
    static_assert(std::is_nothrow_move_constructible_v<NoexceptToggle<true>>);
    static_assert(!std::is_nothrow_move_constructible_v<NoexceptToggle<false>>);
    // 两者都可拷贝（move_if_noexcept 需要"另一条路"）
    static_assert(std::is_copy_constructible_v<NoexceptToggle<true>>);
    static_assert(std::is_copy_constructible_v<NoexceptToggle<false>>);

    NoexceptToggle<false> a(9);
    NoexceptToggle<false> b(std::move(a));
    EXPECT_EQ(b.value, 9);
}

TEST(NewCounterFixture, CountsAllocations) {
    NewCounter::reset();
    int* p = new int(42);
    long n = NewCounter::allocs();
    delete p;
    // 易错点: 计数窗口要尽量窄——中间任何一次标准库分配（std::string、vector、gtest 内部）
    //         都会被算进去。所以 reset() 紧跟被测操作，读取紧跟在后面。
    EXPECT_GE(n, 1);
}

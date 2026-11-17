#include <gtest/gtest.h>

#include <memory>
#include <type_traits>
#include <utility>

#include "common/leak_check.hpp"
#include "common/move_only.hpp"
#include "common/new_counter.hpp"
#include "common/noexcept_toggle.hpp"
#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"

TEST(Fixtures, TracerCountsAndBalances) {
    dojo::test::Tracer::reset();
    {
        dojo::test::Tracer a;
        dojo::test::Tracer b(a);
        dojo::test::Tracer c(std::move(b));
        c = a;
        dojo::test::Tracer d(std::move(a));
        EXPECT_EQ(dojo::test::Tracer::live, 4);
    }
    EXPECT_EQ(dojo::test::Tracer::live, 0);
    EXPECT_GT(dojo::test::Tracer::copy_ctor, 0);
    EXPECT_GT(dojo::test::Tracer::move_ctor, 0);
    EXPECT_EQ(dojo::test::Tracer::total_ctor(), dojo::test::Tracer::dtor);
}

TEST(Fixtures, ThrowOnNInjectsAtNthConstruction) {
    dojo::test::ThrowOnN::disarm();
    dojo::test::ThrowOnN::arm(2);
    EXPECT_THROW(
        {
            dojo::test::ThrowOnN first;
            dojo::test::ThrowOnN second;
        },
        dojo::test::ThrowOnNError);
    dojo::test::ThrowOnN::disarm();
}

TEST(Fixtures, MoveOnlyIsMoveOnly) {
    static_assert(!std::is_copy_constructible_v<dojo::test::MoveOnly>);
    static_assert(std::is_nothrow_move_constructible_v<dojo::test::MoveOnly>);
    dojo::test::MoveOnly a(7);
    dojo::test::MoveOnly b(std::move(a));
    EXPECT_EQ(b.value(), 7);
}

TEST(Fixtures, NoexceptToggleControlsMoveNoexcept) {
    static_assert(std::is_nothrow_move_constructible_v<dojo::test::NoexceptToggle<true>>);
    static_assert(!std::is_nothrow_move_constructible_v<dojo::test::NoexceptToggle<false>>);
    static_assert(std::is_copy_constructible_v<dojo::test::NoexceptToggle<false>>);
}

TEST(Fixtures, LeakCheckBalancedScope) {
    dojo::test::Tracer::reset();
    {
        dojo::test::Tracer t;
        dojo::test::LeakCheck check;
    }
    EXPECT_EQ(dojo::test::Tracer::live, 0);
}

TEST(Fixtures, NewCounterCountsAllocations) {
    const long before_allocs = dojo::test::NewCounter::allocs();
    const long before_bytes = dojo::test::NewCounter::bytes();
    int* p = new int(7);
    const long after_allocs = dojo::test::NewCounter::allocs();
    const long after_bytes = dojo::test::NewCounter::bytes();
    const int value = *p;
    delete p;
    EXPECT_EQ(value, 7);
    EXPECT_EQ(after_allocs - before_allocs, 1);
    EXPECT_GE(after_bytes - before_bytes, static_cast<long>(sizeof(int)));
}

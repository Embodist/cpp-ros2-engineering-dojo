#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>

#include "common/leak_check.hpp"
#include "common/move_only.hpp"
#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"
#include "dojo/mini_vector.hpp"

namespace {

template <bool Noexcept>
struct Probe {
    int value = 0;
    static inline long copies = 0;
    static inline long moves = 0;

    Probe() = default;
    explicit Probe(int v) : value(v) {}
    Probe(const Probe& other) : value(other.value) { ++copies; }
    Probe(Probe&& other) noexcept(Noexcept) : value(other.value) {
        other.value = -1;
        ++moves;
    }
    Probe& operator=(const Probe& other) {
        value = other.value;
        ++copies;
        return *this;
    }
    Probe& operator=(Probe&& other) noexcept(Noexcept) {
        value = other.value;
        other.value = -1;
        ++moves;
        return *this;
    }
};

}  // namespace

TEST(MiniVector, PushAccessGrowth) {
    dojo::MiniVector<int> values;
    EXPECT_EQ(values.size(), 0u);
    for (int i = 0; i < 20; ++i) values.push_back(i);
    EXPECT_EQ(values.size(), 20u);
    EXPECT_GE(values.capacity(), 20u);
    EXPECT_EQ(values.at(10), 10);
    EXPECT_THROW(values.at(20), std::out_of_range);
}

TEST(MiniVector, CopyIsDeep) {
    dojo::MiniVector<std::string> original;
    original.push_back("hello");
    auto copy = original;
    copy.at(0) = "other";
    EXPECT_EQ(original.at(0), "hello");
    EXPECT_EQ(copy.at(0), "other");
}

TEST(MiniVector, MoveOnlyElements) {
    dojo::MiniVector<dojo::test::MoveOnly> values;
    values.push_back(dojo::test::MoveOnly(4));
    values.push_back(dojo::test::MoveOnly(9));
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values.at(1).value(), 9);
}

TEST(MiniVector, MoveContainer) {
    dojo::MiniVector<int> source;
    source.push_back(5);
    auto target = std::move(source);
    EXPECT_EQ(target.at(0), 5);
    EXPECT_EQ(source.size(), 0u);
}

TEST(MiniVector, ReserveFailureRetainsElements) {
    dojo::test::ThrowOnN::disarm();
    dojo::MiniVector<dojo::test::ThrowOnN> values;
    values.push_back(dojo::test::ThrowOnN(1));
    values.push_back(dojo::test::ThrowOnN(2));
    const auto old_capacity = values.capacity();

    dojo::test::ThrowOnN::arm(1);
    EXPECT_THROW(values.reserve(old_capacity + 10), dojo::test::ThrowOnNError);
    dojo::test::ThrowOnN::disarm();

    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values.at(0).value, 1);
    EXPECT_EQ(values.at(1).value, 2);
}

TEST(MiniVector, AliasPushBackOnRealloc) {
    dojo::MiniVector<std::string> values;
    values.push_back("alpha");
    values.push_back(values.at(0));
    ASSERT_EQ(values.size(), 2u);
    EXPECT_EQ(values.at(0), "alpha");
    EXPECT_EQ(values.at(1), "alpha");
}

TEST(MiniVector, SelfAssignment) {
    dojo::MiniVector<std::string> values;
    values.push_back("a");
    values = values;
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values.at(0), "a");
}

TEST(MiniVector, TracerBalancedAndLeakFree) {
    dojo::test::Tracer::reset();
    {
        dojo::test::LeakCheck check;
        dojo::MiniVector<dojo::test::Tracer> values;
        for (int i = 0; i < 32; ++i) values.push_back(dojo::test::Tracer(i));
        EXPECT_EQ(values.size(), 32u);
    }
    EXPECT_EQ(dojo::test::Tracer::live, 0);
    EXPECT_EQ(dojo::test::Tracer::total_ctor(), dojo::test::Tracer::dtor);
}

TEST(MiniVector, NoexceptMoveChoosesMoveOrCopy) {
    Probe<true>::copies = 0;
    Probe<true>::moves = 0;
    {
        dojo::MiniVector<Probe<true>> values;
        for (int i = 0; i < 32; ++i) values.push_back(Probe<true>(i));
        EXPECT_EQ(Probe<true>::copies, 0);
        EXPECT_GT(Probe<true>::moves, 0);
    }

    Probe<false>::copies = 0;
    Probe<false>::moves = 0;
    {
        dojo::MiniVector<Probe<false>> values;
        for (int i = 0; i < 32; ++i) values.push_back(Probe<false>(i));
        EXPECT_GT(Probe<false>::copies, 0);
    }
}

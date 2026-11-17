// 项目 5 参考测试：Vector<T, Alloc>（旗舰）
#include <gtest/gtest.h>

#include <memory>
#include <random>
#include <string>
#include <vector>

#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"
#include "vector.hpp"

// ============================ 迭代器 / 类型性质 ============================
TEST(Vector, ContiguousIteratorAndLayout) {
    static_assert(std::contiguous_iterator<Vector<int>::iterator>);
    static_assert(std::contiguous_iterator<Vector<int>::const_iterator>);
    static_assert(std::is_nothrow_move_constructible_v<Vector<int>>);   // 移动必须 noexcept
    SUCCEED();
}

TEST(Vector, EmptyVectorBeginEqualsEnd) {
    Vector<int> v;
    EXPECT_TRUE(v.empty());
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.begin(), v.end());
    EXPECT_EQ(v.capacity(), 0u);
}

// ============================ 容量语义 ============================
TEST(Vector, ClearAndResizeShrinkKeepCapacity) {
    Vector<int> v;
    v.reserve(64);
    for (int i = 0; i < 10; ++i) v.push_back(i);
    const auto cap = v.capacity();

    v.resize(3);                              // 缩小：只析构，不释放容量
    EXPECT_EQ(v.size(), 3u);
    EXPECT_EQ(v.capacity(), cap);

    v.clear();
    EXPECT_EQ(v.size(), 0u);
    EXPECT_EQ(v.capacity(), cap);
    EXPECT_EQ(v.begin(), v.end());
}

TEST(Vector, ReserveNeverShrinks) {
    Vector<int> v{1, 2, 3};
    v.reserve(100);
    const auto cap = v.capacity();
    EXPECT_GE(cap, 100u);
    v.reserve(10);                            // 小于当前容量 → 不动
    EXPECT_EQ(v.capacity(), cap);
    v.shrink_to_fit();
    EXPECT_EQ(v.capacity(), v.size());        // shrink_to_fit 收缩到 size
}

// ============================ 别名安全 ============================
TEST(Vector, PushBackSelfElementAliasWithAndWithoutRealloc) {
    Vector<int> v{1, 2, 3};
    v.reserve(10);                            // 无扩容路径
    v.push_back(v[0]);
    EXPECT_EQ(v.size(), 4u);
    EXPECT_EQ(v[3], 1);

    Vector<int> w{7, 8, 9};                   // 恰好满容量 → 触发扩容
    w.push_back(w[0]);
    EXPECT_EQ(w.size(), 4u);
    EXPECT_EQ(w[0], 7);
    EXPECT_EQ(w[3], 7);                       // 扩容搬移后原值仍在

    // 用 std::string（nothrow 移动）：扩容会走 move，若实现先搬旧元素再读实参就会读到空串
    Vector<std::string> vs{"alpha", "beta"};
    vs.push_back(vs[0]);
    EXPECT_EQ(vs[0], "alpha");
    EXPECT_EQ(vs[2], "alpha");
}

TEST(Vector, InsertSelfElementAlias) {
    Vector<int> v{1, 2, 3, 4};
    v.reserve(10);                            // 无扩容
    v.insert(v.begin() + 2, v[0]);
    EXPECT_EQ(v, (Vector<int>{1, 2, 1, 3, 4}));

    Vector<int> w{1, 2, 3};                   // 扩容
    w.insert(w.begin(), w[2]);
    EXPECT_EQ(w, (Vector<int>{3, 1, 2, 3}));

    Vector<std::string> s{"a", "b", "c"};
    s.insert(s.begin() + 1, s[0]);            // 扩容 + nothrow 移动
    EXPECT_EQ(s, (Vector<std::string>{"a", "a", "b", "c"}));
}

// ============================ 自赋值 / 自 swap ============================
TEST(Vector, SelfAssignmentAndSelfSwap) {
    Vector<int> v{1, 2, 3};
    Vector<int>& ref = v;                     // 绕开 -Wself-move 的编译告警
    v = ref;                                  // 自拷贝赋值
    EXPECT_EQ(v, (Vector<int>{1, 2, 3}));
    v = std::move(ref);                       // 自移动赋值
    EXPECT_EQ(v, (Vector<int>{1, 2, 3}));
    v.swap(v);                                // 自 swap
    EXPECT_EQ(v, (Vector<int>{1, 2, 3}));
}

// ============================ 异常安全 ============================
TEST(Vector, StrongGuaranteeOnThrowingEmplaceNoRealloc) {
    Vector<ThrowOnN> v;
    v.reserve(8);
    for (int i = 0; i < 4; ++i) v.emplace_back(i);

    ThrowOnN::arm(1);                         // 下一次构造就抛
    EXPECT_THROW(v.emplace_back(99), ThrowOnNError);
    ThrowOnN::disarm();

    ASSERT_EQ(v.size(), 4u);                  // 强保证：原数据不变
    for (int i = 0; i < 4; ++i) EXPECT_EQ(v[i].value, i);
}

TEST(Vector, ThrowOnNScanAcrossReallocation) {
    // 不预分配 → 每次 emplace_back 都可能扩容。逐个 arm(k) 扫描每个抛出点：
    // 只要抛了，原数据必须逐元素不变（强保证），且不泄漏（ASan 兜底）。
    for (int arm = 1; arm <= 12; ++arm) {
        Vector<ThrowOnN> v;
        for (int i = 0; i < 4; ++i) v.emplace_back(i);
        const auto before = v.size();

        ThrowOnN::arm(arm);
        bool threw = false;
        try { v.emplace_back(99); } catch (const ThrowOnNError&) { threw = true; }
        ThrowOnN::disarm();

        if (threw) {
            ASSERT_EQ(v.size(), before) << "arm=" << arm;
            for (int i = 0; i < 4; ++i) ASSERT_EQ(v[i].value, i) << "arm=" << arm;
        }
    }
}

TEST(Vector, CopyCtorAndAssignStrongGuarantee) {
    Vector<ThrowOnN> src;
    src.emplace_back(1);
    src.emplace_back(2);

    ThrowOnN::arm(2);                         // 拷贝第 2 个元素时抛
    EXPECT_THROW({ Vector<ThrowOnN> c(src); }, ThrowOnNError);
    ThrowOnN::disarm();
    EXPECT_EQ(src.size(), 2u);                // 源不变

    Vector<ThrowOnN> dst;
    dst.emplace_back(0);
    ThrowOnN::arm(1);
    EXPECT_THROW(dst = src, ThrowOnNError);   // copy-and-swap → 强保证
    ThrowOnN::disarm();
    ASSERT_EQ(dst.size(), 1u);
    EXPECT_EQ(dst[0].value, 0);
}

// ============================ 扩容的移动 vs 拷贝 ============================
namespace {
template <bool Noexcept>
struct MoveProbe {
    int v = 0;
    static inline long copies = 0;
    static inline long moves  = 0;
    MoveProbe() = default;
    explicit MoveProbe(int x) : v(x) {}
    MoveProbe(const MoveProbe& o) : v(o.v) { ++copies; }
    MoveProbe(MoveProbe&& o) noexcept(Noexcept) : v(o.v) { ++moves; }
    MoveProbe& operator=(const MoveProbe& o) { v = o.v; ++copies; return *this; }
    MoveProbe& operator=(MoveProbe&& o) noexcept(Noexcept) { v = o.v; ++moves; return *this; }
};
}

TEST(Vector, GrowthMovesWhenNoexceptOtherwiseCopies) {
    // move_if_noexcept 的核心行为：移动"可能抛"时退回拷贝，以免搬了一半失败丢数据。
    MoveProbe<true>::copies = MoveProbe<true>::moves = 0;
    {
        Vector<MoveProbe<true>> v;
        for (int i = 0; i < 64; ++i) v.emplace_back(i);
        EXPECT_GT(MoveProbe<true>::moves, 0);
        EXPECT_EQ(MoveProbe<true>::copies, 0);    // noexcept 移动 → 选移动
    }
    MoveProbe<false>::copies = MoveProbe<false>::moves = 0;
    {
        Vector<MoveProbe<false>> v;
        for (int i = 0; i < 64; ++i) v.emplace_back(i);
        EXPECT_EQ(MoveProbe<false>::moves, 0);
        EXPECT_GT(MoveProbe<false>::copies, 0);   // 可抛移动 → 退回拷贝
    }
}

TEST(Vector, PushBackAmortizedLinear) {
    // 摊还 O(1)：n 次 push_back 的总搬移次数被增长因子约束为 c·n。
    Tracer::reset();
    {
        Vector<Tracer> v;
        const long n = 20000;
        for (long i = 0; i < n; ++i) v.emplace_back(static_cast<int>(i));
        EXPECT_EQ(Tracer::copy_ctor, 0);          // Tracer 可 noexcept 移动 → 扩容只搬移不拷贝
        EXPECT_LE(Tracer::move_ctor, 4 * n);      // 增长因子 1.5 → 总搬移 ≈ 3n，上界取 4n
        EXPECT_EQ(Tracer::live, n);
    }
    EXPECT_EQ(Tracer::live, 0);                    // 构造数 == 析构数
}

// ============================ move-only 元素 ============================
TEST(Vector, MoveOnlyUniquePtrElements) {
    Vector<std::unique_ptr<int>> v;
    v.push_back(std::make_unique<int>(1));
    v.emplace_back(new int(2));
    v.insert(v.begin(), std::make_unique<int>(0));
    ASSERT_EQ(v.size(), 3u);
    EXPECT_EQ(*v[0], 0);
    EXPECT_EQ(*v[1], 1);
    EXPECT_EQ(*v[2], 2);
    v.erase(v.begin());
    EXPECT_EQ(*v[0], 1);
}

// ============================ 拷贝 / 移动语义 ============================
TEST(Vector, CopyIndependentMoveEmpties) {
    Vector<int> a{1, 2, 3};
    Vector<int> b = a;                        // 深拷贝
    b.push_back(4);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(b.size(), 4u);

    Vector<int> c = std::move(a);             // 移动
    EXPECT_EQ(c, (Vector<int>{1, 2, 3}));
    EXPECT_TRUE(a.empty());                   // 源为空但仍可用
    a.push_back(9);
    EXPECT_EQ(a.size(), 1u);
}

// ============================ 非成员算法 / 比较 ============================
TEST(Vector, NonMemberEraseEraseIfAndCompare) {
    Vector<int> v{1, 2, 3, 2, 4, 2};
    EXPECT_EQ(erase(v, 2), 3u);
    EXPECT_EQ(v, (Vector<int>{1, 3, 4}));

    Vector<int> w{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(erase_if(w, [](int x) { return x % 2 == 0; }), 3u);
    EXPECT_EQ(w, (Vector<int>{1, 3, 5}));

    EXPECT_TRUE((Vector<int>{1, 2, 3} < Vector<int>{1, 2, 4}));   // <=> 派生出的 <
    EXPECT_TRUE((Vector<int>{1, 2} == Vector<int>{1, 2}));
}

TEST(Vector, InsertBoundaryCases) {
    Vector<int> v{1, 2, 3};
    auto it = v.insert(v.end(), 4);           // pos == end → 等价 push_back
    EXPECT_EQ(*it, 4);
    EXPECT_EQ(v.back(), 4);

    it = v.insert(v.begin() + 1, 0, 99);      // n == 0 → 原地返回
    EXPECT_EQ(*it, 2);
    EXPECT_EQ(v, (Vector<int>{1, 2, 3, 4}));

    std::vector<int> src{7, 8, 9};
    v.insert(v.begin() + 2, src.begin(), src.end());
    EXPECT_EQ(v, (Vector<int>{1, 2, 7, 8, 9, 3, 4}));
}

// ============================ 差分测试：对照 std::vector ============================
TEST(Vector, DifferentialAgainstStdVector) {
    std::mt19937 rng(2027);
    Vector<int> a;
    std::vector<int> b;

    auto sync = [&] {
        ASSERT_EQ(a.size(), b.size());
        for (std::size_t i = 0; i < b.size(); ++i) ASSERT_EQ(a[i], b[i]);
    };

    for (int step = 0; step < 100000; ++step) {
        switch (rng() % 8) {
            case 0: { int v = static_cast<int>(rng() % 100); a.push_back(v); b.push_back(v); break; }
            case 1: if (!b.empty()) { a.pop_back(); b.pop_back(); } break;
            case 2: if (b.size() < 50) {
                int v = static_cast<int>(rng() % 100);
                std::size_t p = rng() % (b.size() + 1);
                a.insert(a.begin() + static_cast<std::ptrdiff_t>(p), v);
                b.insert(b.begin() + static_cast<std::ptrdiff_t>(p), v);
            } break;
            case 3: if (!b.empty()) {
                std::size_t p = rng() % b.size();
                a.erase(a.begin() + static_cast<std::ptrdiff_t>(p));
                b.erase(b.begin() + static_cast<std::ptrdiff_t>(p));
            } break;
            case 4: a.reserve(a.size() + (rng() % 8)); break;
            case 5: if (b.size() > 100) { a.clear(); b.clear(); } break;
            case 6: { std::size_t n = rng() % 20; a.resize(n); b.resize(n); } break;
            case 7: if (!b.empty() && b.size() < 80) {
                std::size_t p = rng() % b.size();
                int v = static_cast<int>(rng() % 100);
                a.insert(a.begin() + static_cast<std::ptrdiff_t>(p), 2, v);
                b.insert(b.begin() + static_cast<std::ptrdiff_t>(p), 2, v);
            } break;
        }
        sync();
    }
}

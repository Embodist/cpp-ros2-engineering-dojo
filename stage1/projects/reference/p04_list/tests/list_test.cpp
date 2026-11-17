// 项目 4 参考测试：List<T>（哨兵双向链表）
#include <gtest/gtest.h>

#include <algorithm>
#include <iterator>
#include <list>
#include <random>
#include <type_traits>
#include <vector>

#include "common/move_only.hpp"
#include "common/throw_on_n.hpp"
#include "common/tracer.hpp"
#include "list.hpp"

// ============================ 迭代器性质 ============================
TEST(List, IteratorIsBidirectionalAndOneWayConvertible) {
    // 内功核查点：迭代器类别名副其实（头文件里也有同样的 static_assert）
    static_assert(std::bidirectional_iterator<List<int>::iterator>);
    static_assert(std::bidirectional_iterator<List<int>::const_iterator>);
    // iterator → const_iterator 可隐式转换；反向【不可】——这正是 const 正确性想要的
    static_assert(std::is_convertible_v<List<int>::iterator, List<int>::const_iterator>);
    static_assert(!std::is_convertible_v<List<int>::const_iterator, List<int>::iterator>);

    List<int> l{1, 2, 3};
    List<int>::iterator it = l.begin();
    List<int>::const_iterator cit = it;      // 隐式转换
    EXPECT_EQ(*cit, 1);
    EXPECT_EQ(cit, it);                      // 混合比较
    ++cit;
    EXPECT_EQ(*cit, 2);
    EXPECT_EQ(*it, 1);                       // 原 iterator 不受影响
}

TEST(List, ReverseIteratorWalksBackwards) {
    List<int> l{1, 2, 3};
    std::vector<int> got;
    for (auto it = l.rbegin(); it != l.rend(); ++it) got.push_back(*it);
    EXPECT_EQ(got, (std::vector<int>{3, 2, 1}));
}

// ============================ 基本修改 / 哨兵 ============================
TEST(List, SentinelMakesEmptyListUniform) {
    // 训练点：哨兵让"空表"没有特例。begin()==end() 天然成立；
    //         push_back 到空表 = 在哨兵前插一个节点，与插入到非空表【同一份代码】。
    List<int> l;
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.begin(), l.end());           // 空表：首 == 尾（都指哨兵）

    l.push_back(10);
    EXPECT_EQ(l.size(), 1u);
    EXPECT_EQ(l.front(), 10);
    EXPECT_EQ(l.back(), 10);
    l.pop_back();
    EXPECT_TRUE(l.empty());
    EXPECT_EQ(l.begin(), l.end());

    l.push_front(20);                        // push_front 到空表同样无需特判
    EXPECT_EQ(l.front(), 20);
    EXPECT_EQ(l.back(), 20);
}

TEST(List, InsertErasePushPop) {
    List<int> l{1, 2, 3, 4, 5};
    EXPECT_EQ(l.size(), 5u);

    auto it = l.begin();
    std::advance(it, 2);                     // 指向 3
    auto r = l.insert(it, 99);               // 3 之前插 99
    EXPECT_EQ(*r, 99);
    EXPECT_EQ(l, (List<int>{1, 2, 99, 3, 4, 5}));

    it = l.begin();
    std::advance(it, 2);                     // 指向 99
    auto next = l.erase(it);                 // erase 返回下一个
    EXPECT_EQ(*next, 3);
    EXPECT_EQ(l, (List<int>{1, 2, 3, 4, 5}));
}

TEST(List, InsertCountAndRangeReturnFirst) {
    List<int> l{1, 5};
    auto it = l.begin();
    ++it;                                    // 指向 5
    auto r = l.insert(it, 3, 7);             // {1,7,7,7,5}
    EXPECT_EQ(*r, 7);
    EXPECT_EQ(l, (List<int>{1, 7, 7, 7, 5}));

    std::vector<int> v{8, 9};
    auto r2 = l.insert(l.end(), v.begin(), v.end());
    EXPECT_EQ(*r2, 8);
    EXPECT_EQ(l.size(), 7u);
    EXPECT_EQ(l.back(), 9);
}

TEST(List, EraseRangeReturnsLast) {
    List<int> l{1, 2, 3, 4, 5, 6};
    auto f = l.begin(); std::advance(f, 1);
    auto t = l.begin(); std::advance(t, 4);  // 区间 [2,3,4) → 下标 1,2,3
    auto r = l.erase(f, t);
    EXPECT_EQ(*r, 5);
    EXPECT_EQ(l, (List<int>{1, 5, 6}));
}

// ============================ 失效规则 ============================
TEST(List, EraseInvalidatesOnlyTheErasedIterator) {
    List<int> l{1, 2, 3, 4, 5};
    auto it2 = l.begin(); ++it2;             // 指向 2
    auto it3 = l.begin(); std::advance(it3, 2);
    auto it5 = l.begin(); std::advance(it5, 4);
    int& ref5 = *it5;

    l.erase(it3);                            // 只删 3

    // 训练点：链表 erase 只让【被删的】迭代器失效，其余迭代器与引用照常有效。
    EXPECT_EQ(*it2, 2);
    EXPECT_EQ(*it5, 5);
    EXPECT_EQ(ref5, 5);
    EXPECT_EQ(l, (List<int>{1, 2, 4, 5}));
}

TEST(List, SpliceKeepsIteratorsValidAndRebindsToList) {
    List<int> a{1, 2, 3};
    List<int> b{10, 20};
    auto it = a.begin(); ++it;               // 指向 a 里的 2

    b.splice(b.end(), a, it);                // 把节点 2 搬到 b 末尾

    // 训练点：splice 不拷贝元素、不失效迭代器——it 依旧指向那个节点，只是"归属"变成了 b。
    EXPECT_EQ(*it, 2);
    ++it;
    EXPECT_EQ(it, b.end());                  // 现在沿着 b 走能走到 b.end()
    EXPECT_EQ(a, (List<int>{1, 3}));
    EXPECT_EQ(b, (List<int>{10, 20, 2}));
}

TEST(List, SpliceWholeListAndRange) {
    List<int> a{1, 2, 3, 4, 5};
    List<int> b;
    auto f = a.begin(); std::advance(f, 1);
    auto t = a.begin(); std::advance(t, 3);  // [2,3)
    b.splice(b.end(), a, f, t);              // b={2,3}, a={1,4,5}
    EXPECT_EQ(a, (List<int>{1, 4, 5}));
    EXPECT_EQ(b, (List<int>{2, 3}));

    List<int> c{100};
    c.splice(c.begin(), b);                  // 整表搬到 c 头部 → {2,3,100}
    EXPECT_EQ(c, (List<int>{2, 3, 100}));
    EXPECT_TRUE(b.empty());
}

// ============================ 异常安全 ============================
TEST(List, ElementConstructionFailureFreesNode) {
    List<ThrowOnN> l;
    l.emplace_back(1);
    l.emplace_back(2);
    ASSERT_EQ(l.size(), 2u);

    ThrowOnN::arm(1);                        // 下一次构造就抛
    EXPECT_THROW(l.emplace_back(3), ThrowOnNError);
    ThrowOnN::disarm();

    EXPECT_EQ(l.size(), 2u);                 // 强保证：规模不变
    // 若 create_node 忘记在异常时释放节点，这里会泄漏 → 由 ASan/LeakSanitizer 在进程退出时报出。
}

// ============================ 大节点量：迭代析构 ============================
TEST(List, DestroysManyNodesWithoutStackOverflow) {
    // 训练点：10 万节点若用 unique_ptr 链，析构会递归 10 万层 → 爆栈段错误。
    //         迭代析构把递归深度降到 O(1)，这里能顺利跑完即是证据。
    {
        List<int> l;
        for (int i = 0; i < 100000; ++i) l.push_back(i);
        EXPECT_EQ(l.size(), 100000u);
        EXPECT_EQ(l.front(), 0);
        EXPECT_EQ(l.back(), 99999);
    }
    SUCCEED();
}

// ============================ reverse / sort ============================
TEST(List, ReverseIsZeroCopy) {
    List<Tracer> l;
    for (int i = 1; i <= 5; ++i) l.emplace_back(i);
    // 用"前后差值"测拷贝/移动：不建议在还有存活 Tracer 时调 Tracer::reset()，
    // 那会把 live 强行清零，之后再析构会算成负数。
    const long before = Tracer::total_copy() + Tracer::total_move();
    l.reverse();
    EXPECT_EQ(Tracer::total_copy() + Tracer::total_move(), before);   // 零拷贝零移动
    int expect = 5;
    for (const Tracer& t : l) EXPECT_EQ(t.value(), expect--);
}

TEST(List, SortIsStable) {
    struct Rec { int key; int seq; };
    List<Rec> l;
    int seq = 0;
    for (int k : {3, 1, 2, 1, 3, 2, 1}) l.push_back(Rec{k, seq++});

    l.sort([](const Rec& x, const Rec& y) { return x.key < y.key; });

    // 稳定性：键相等时，原始 seq 必须保持升序（相对顺序不变）
    int last_key = -1, last_seq = -1;
    for (const Rec& r : l) {
        if (r.key == last_key) { EXPECT_LT(last_seq, r.seq); }
        EXPECT_GE(r.key, last_key);
        last_key = r.key;
        last_seq = r.seq;
    }
}

TEST(List, SortIsZeroCopy) {
    List<Tracer> l;
    for (int v : {5, 2, 8, 1, 9, 3}) l.emplace_back(v);
    const long before = Tracer::total_copy() + Tracer::total_move();
    l.sort([](const Tracer& a, const Tracer& b) { return a.value() < b.value(); });
    // 训练点：只重连节点、比较用引用，排序全程【零元素拷贝/移动】。
    EXPECT_EQ(Tracer::total_copy() + Tracer::total_move(), before);
    EXPECT_EQ(Tracer::live, 6);              // 元素个数不变
    int prev = -1;                           // 值应严格递增（输入为 5,2,8,1,9,3）
    for (const Tracer& t : l) { EXPECT_GT(t.value(), prev); prev = t.value(); }
    EXPECT_EQ(prev, 9);
}

TEST(List, SortManyElementsMatchesStdSort) {
    std::mt19937 rng(42);
    std::vector<int> ref;
    List<int> l;
    for (int i = 0; i < 5000; ++i) { int v = static_cast<int>(rng() % 1000); ref.push_back(v); l.push_back(v); }
    std::sort(ref.begin(), ref.end());
    l.sort();
    auto it = l.begin();
    for (int v : ref) { ASSERT_EQ(*it, v); ++it; }
    EXPECT_EQ(it, l.end());
}

// ============================ merge / unique / remove_if ============================
TEST(List, MergeIsStableAndRelinks) {
    struct Rec { int key; int seq; };
    List<Rec> a, b;
    a.push_back(Rec{1, 0}); a.push_back(Rec{3, 1});
    b.push_back(Rec{1, 2}); b.push_back(Rec{3, 3});   // 与 a 的键相同
    a.merge(b, [](const Rec& x, const Rec& y) { return x.key < y.key; });

    EXPECT_TRUE(b.empty());
    EXPECT_EQ(a.size(), 4u);
    // 稳定：相等键时 a 的元素在前 → seq 顺序应为 0,2,1,3
    std::vector<int> seqs;
    for (const Rec& r : a) seqs.push_back(r.seq);
    EXPECT_EQ(seqs, (std::vector<int>{0, 2, 1, 3}));
}

TEST(List, UniqueRemovesConsecutiveDuplicates) {
    List<int> l{1, 1, 2, 2, 2, 3, 1};
    EXPECT_EQ(l.unique(), 3u);               // 删掉多余的 1,2,2
    EXPECT_EQ(l, (List<int>{1, 2, 3, 1}));
}

TEST(List, RemoveIfAndRemove) {
    List<int> l{1, 2, 3, 4, 5, 6};
    EXPECT_EQ(l.remove_if([](int x) { return x % 2 == 0; }), 3u);
    EXPECT_EQ(l, (List<int>{1, 3, 5}));
    l.push_front(3);
    EXPECT_EQ(l.remove(3), 2u);
    EXPECT_EQ(l, (List<int>{1, 5}));
}

// ============================ 纯移动元素 ============================
TEST(List, SupportsMoveOnlyElements) {
    // 训练点：List 全程只搬移不拷贝，因此 move-only 元素天然可用。
    //         任何隐藏的拷贝都会让这段代码【编译失败】——这就是证明。
    List<MoveOnly> l;
    l.push_back(MoveOnly(1));
    l.emplace_back(2);
    l.emplace_front(0);
    std::vector<int> vals;
    for (const MoveOnly& m : l) vals.push_back(m.value());
    EXPECT_EQ(vals, (std::vector<int>{0, 1, 2}));

    l.sort([](const MoveOnly& a, const MoveOnly& b) { return a.value() < b.value(); });
    EXPECT_EQ(l.size(), 3u);
}

// ============================ 拷贝 / 移动 / 初始化列表 ============================
TEST(List, CopyIsDeepMoveSteals) {
    List<int> a{1, 2, 3};
    List<int> b = a;                         // 深拷贝
    b.push_back(4);
    EXPECT_EQ(a.size(), 3u);
    EXPECT_EQ(b.size(), 4u);
    EXPECT_EQ(a, (List<int>{1, 2, 3}));

    List<int> c = std::move(a);              // 移动：O(1) 偷链
    EXPECT_TRUE(a.empty());
    EXPECT_EQ(c, (List<int>{1, 2, 3}));
}

TEST(List, InitializerListAndRangeConstruction) {
    List<int> a{1, 2, 3, 4};
    EXPECT_EQ(a.size(), 4u);
    std::vector<int> v{5, 6, 7};
    List<int> b(v.begin(), v.end());
    EXPECT_EQ(b, (List<int>{5, 6, 7}));
}

// ============================ 差分测试：对照 std::list ============================
TEST(List, DifferentialAgainstStdList) {
    std::mt19937 rng(2026);
    List<int> a;
    std::list<int> b;

    auto sync = [&] {
        ASSERT_EQ(a.size(), b.size());
        auto ia = a.begin();
        for (int v : b) { ASSERT_EQ(*ia, v); ++ia; }
        ASSERT_EQ(ia, a.end());
    };

    for (int step = 0; step < 20000; ++step) {
        switch (rng() % 8) {
            case 0: { int v = static_cast<int>(rng() % 100); a.push_back(v);  b.push_back(v);  break; }
            case 1: { int v = static_cast<int>(rng() % 100); a.push_front(v); b.push_front(v); break; }
            case 2: if (!b.empty()) { a.pop_front(); b.pop_front(); } break;
            case 3: if (!b.empty()) { a.pop_back();  b.pop_back();  } break;
            case 4: if (!b.empty()) {
                std::size_t p = rng() % b.size();
                int v = static_cast<int>(rng() % 100);
                auto ia = a.begin(); std::advance(ia, static_cast<std::ptrdiff_t>(p));
                auto ib = b.begin(); std::advance(ib, static_cast<std::ptrdiff_t>(p));
                a.insert(ia, v); b.insert(ib, v);
            } break;
            case 5: if (!b.empty()) {
                std::size_t p = rng() % b.size();
                auto ia = a.begin(); std::advance(ia, static_cast<std::ptrdiff_t>(p));
                auto ib = b.begin(); std::advance(ib, static_cast<std::ptrdiff_t>(p));
                a.erase(ia); b.erase(ib);
            } break;
            case 6: a.reverse(); b.reverse(); break;
            case 7: if (b.size() > 300) { a.clear(); b.clear(); } break;
        }
        sync();
    }
}

// 项目 2 参考测试：UniquePtr / SharedPtr / WeakPtr / MakeShared
#include <gtest/gtest.h>

#include <atomic>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "common/new_counter.hpp"
#include "common/tracer.hpp"
#include "shared_ptr.hpp"
#include "unique_ptr.hpp"

// ============================ UniquePtr ============================
TEST(UniquePtr, NotCopyableButNothrowMovable) {
    // 内功核查点：编译期性质用 static_assert 证明
    static_assert(!std::is_copy_constructible_v<UniquePtr<int>>);
    static_assert(!std::is_copy_assignable_v<UniquePtr<int>>);
    static_assert(std::is_nothrow_move_constructible_v<UniquePtr<int>>);
    static_assert(std::is_nothrow_move_assignable_v<UniquePtr<int>>);
    // EBO / no_unique_address：空删除器不占空间
    static_assert(sizeof(UniquePtr<int>) == sizeof(int*));
    SUCCEED();
}

TEST(UniquePtr, MoveNullsSource) {
    auto a = MakeUnique<int>(42);
    ASSERT_TRUE(static_cast<bool>(a));
    UniquePtr<int> b(std::move(a));
    EXPECT_EQ(*b, 42);
    EXPECT_EQ(a.get(), nullptr);   // 移动后源为空
    EXPECT_FALSE(static_cast<bool>(a));
}

TEST(UniquePtr, ReleaseAndReset) {
    Tracer::reset();
    {
        auto a = MakeUnique<Tracer>(1);
        Tracer* raw = a.release();        // 交出所有权，不析构
        EXPECT_EQ(Tracer::dtor, 0);
        EXPECT_EQ(a.get(), nullptr);
        UniquePtr<Tracer> b(raw);         // 重新接管
        EXPECT_EQ(Tracer::live, 1);
    }                                     // b 析构 → 调用删除器
    EXPECT_EQ(Tracer::live, 0);
    EXPECT_EQ(Tracer::dtor, 1);

    Tracer::reset();
    {
        auto c = MakeUnique<Tracer>(2);
        c.reset(new Tracer(3));           // 释放旧对象、接管新对象
        EXPECT_EQ(Tracer::dtor, 1);
        EXPECT_EQ(Tracer::live, 1);
    }
    EXPECT_EQ(Tracer::live, 0);
}

TEST(UniquePtr, SelfMoveAssignIsSafe) {
    auto a = MakeUnique<int>(7);
    int* p = a.get();
    // 用引用绕一下，避免编译器直接看出"自移动"而报 -Wself-move（这里是有意测试该场景）
    UniquePtr<int>& ref = a;
    a = std::move(ref);           // 自移动赋值不应自我释放
    EXPECT_EQ(a.get(), p);
    EXPECT_EQ(*a, 7);
}

TEST(UniquePtr, CustomDeleterStatefulLambda) {
    int calls = 0;
    {
        UniquePtr<int, std::function<void(int*)>> up(new int(1),
            [&calls](int* p) { ++calls; delete p; });
    }
    EXPECT_EQ(calls, 1);
}

TEST(UniquePtr, CustomDeleterManagesFile) {
    const std::string path = (std::filesystem::temp_directory_path() / "p02_file.txt").string();
    {
        FILE* f = std::fopen(path.c_str(), "w");
        ASSERT_NE(f, nullptr);
        // 易错点: deleter 必须捕获 fclose 的语义——UniquePtr<FILE> 默认删的是 FILE*（错），
        //         必须传自定义删除器，否则 fopen 的资源不会正确释放。
        UniquePtr<FILE, void (*)(FILE*)> fp(f, [](FILE* p) { std::fclose(p); });
        ASSERT_TRUE(static_cast<bool>(fp));
    }
    std::filesystem::remove(path);
    SUCCEED();
}

TEST(UniquePtr, ArraySpecializationUsesDeleteBracket) {
    Tracer::reset();
    {
        auto arr = MakeUnique<Tracer[]>(5);
        EXPECT_EQ(Tracer::live, 5);        // 5 个元素都构造了
        EXPECT_EQ(arr[0].value(), 0);     // operator[] 可用
    }
    // 易错点: delete[] 必须销毁【全部】元素。若误用 delete（单个），只有首元素析构 → 这里就会露。
    EXPECT_EQ(Tracer::live, 0);
    EXPECT_EQ(Tracer::dtor, 5);
}

TEST(UniquePtr, DerivedToBaseMove) {
    struct Base { virtual ~Base() = default; int b = 1; };
    struct Derived : Base { int d = 2; };
    UniquePtr<Derived> dp(new Derived);
    UniquePtr<Base> bp(std::move(dp));    // 派生 → 基类
    EXPECT_EQ(bp->b, 1);
    EXPECT_EQ(dp.get(), nullptr);
}

TEST(UniquePtr, DifferentialWithStdUniquePtr) {
    auto a = MakeUnique<int>(5);
    auto b = std::make_unique<int>(5);
    EXPECT_EQ(*a, *b);
    a.reset(); b.reset();
    EXPECT_EQ(a.get() == nullptr, b.get() == nullptr);

    int* pa = new int(9);
    int* pb = new int(9);
    UniquePtr<int> ua(pa);
    std::unique_ptr<int> ub(pb);
    EXPECT_EQ(*ua, *ub);
    EXPECT_EQ(ua.release() == pa, ub.release() == pb);
    delete pa; delete pb;
}

// ============================ SharedPtr / WeakPtr ============================
TEST(SharedPtr, CopyIncrementsAndLastRefDestroys) {
    Tracer::reset();
    {
        SharedPtr<Tracer> sp = MakeShared<Tracer>(1);
        EXPECT_EQ(sp.use_count(), 1);
        {
            SharedPtr<Tracer> c = sp;
            EXPECT_EQ(sp.use_count(), 2);
            EXPECT_EQ(c.use_count(), 2);
            EXPECT_EQ(Tracer::live, 1);
        }
        EXPECT_EQ(sp.use_count(), 1);     // 子作用域里的副本析构 → 计数回落
        EXPECT_EQ(Tracer::live, 1);       // 对象还在
    }
    EXPECT_EQ(Tracer::live, 0);           // 最后一个强引用消失 → 对象析构
}

TEST(SharedPtr, WeakDoesNotPreventDestruction) {
    Tracer::reset();
    WeakPtr<Tracer> w;
    {
        SharedPtr<Tracer> sp = MakeShared<Tracer>(1);
        w = sp;
        EXPECT_FALSE(w.expired());
        EXPECT_EQ(w.use_count(), 1);
    }
    // 强引用都没了 → 对象销毁，但控制块被 weak 引用着，所以 w 仍可用（expired）
    EXPECT_TRUE(w.expired());
    EXPECT_FALSE(static_cast<bool>(w.lock()));   // lock 失败返回空
    EXPECT_EQ(Tracer::live, 0);
}

TEST(SharedPtr, WeakLockSucceedsWhileAlive) {
    auto sp = MakeShared<int>(7);
    WeakPtr<int> w = sp;
    {
        auto locked = w.lock();
        ASSERT_TRUE(static_cast<bool>(locked));
        EXPECT_EQ(*locked, 7);
        EXPECT_EQ(sp.use_count(), 2);
    }
    EXPECT_EQ(sp.use_count(), 1);
}

TEST(SharedPtr, MakeSharedAllocatesOnce) {
    // 内功核查点：MakeShared 比 SharedPtr(new T) 少一次分配。
    NewCounter::reset();
    {
        auto sp = MakeShared<int>(1);
        EXPECT_EQ(NewCounter::allocs(), 1);   // 只分配控制块（对象内嵌其中）
    }
    NewCounter::reset();
    {
        SharedPtr<int> sp(new int(2));
        EXPECT_EQ(NewCounter::allocs(), 2);   // 对象一次 + 控制块一次
    }
}

TEST(SharedPtr, CustomDeleterIsTypeErased) {
    const std::string path = (std::filesystem::temp_directory_path() / "p02_file2.txt").string();
    FILE* f = std::fopen(path.c_str(), "w");
    ASSERT_NE(f, nullptr);
    // 删除器类型（lambda）不出现在 SharedPtr<FILE> 的类型里 —— 这就是类型擦除：
    //   控制块用虚函数记住实际删除器，SharedPtr 模板参数只看元素类型。
    static_assert(std::is_same_v<decltype(SharedPtr<FILE>(f, [](FILE* p) { std::fclose(p); })),
                                 SharedPtr<FILE>>);
    {
        SharedPtr<FILE> sp(f, [](FILE* p) { std::fclose(p); });
        EXPECT_EQ(sp.use_count(), 1);
    }
    std::filesystem::remove(path);
    SUCCEED();
}

namespace {
struct Node {
    Tracer tag;                 // 用于观测对象是否析构
    SharedPtr<Node> next;       // 强引用
    WeakPtr<Node> weak_next;    // 弱引用
};
}  // namespace

TEST(SharedPtr, CyclicReferenceLeaks) {
    Tracer::reset();
    auto a = MakeShared<Node>();
    auto b = MakeShared<Node>();
    a->next = b;      // a 强引用 b
    b->next = a;      // b 强引用 a  → 循环引用

    // 用弱引用观察，再"模拟外层 shared_ptr 析构"（把局部引用 reset 掉）
    WeakPtr<Node> wa = a;
    WeakPtr<Node> wb = b;
    a.reset();
    b.reset();

    // 训练点: 环让两个对象互相保命 —— 没有任何外部强引用了，对象却还活着，这就是泄漏。
    EXPECT_EQ(Tracer::live, 2);
    EXPECT_FALSE(wa.expired());
    EXPECT_FALSE(wb.expired());

    // 手动打破环并回收，避免真的泄漏到进程退出（否则 ASan/LeakSanitizer 会报错）
    auto ra = wa.lock();
    auto rb = wb.lock();
    ASSERT_TRUE(ra && rb);
    ra->next.reset();          // 断开 a->b
    rb->next.reset();          // 断开 b->a
    ra.reset();
    rb.reset();
    EXPECT_EQ(Tracer::live, 0);   // 环一断，对象立刻回收
}

TEST(SharedPtr, WeakBreaksCycle) {
    Tracer::reset();
    {
        auto a = MakeShared<Node>();
        auto b = MakeShared<Node>();
        a->weak_next = b;   // 只用弱引用 → 不构成所有权环
        b->weak_next = a;
    }
    EXPECT_EQ(Tracer::live, 0);   // 无泄漏
}

TEST(SharedPtr, ConcurrentCopyAndDestroy) {
    // 多线程拷贝/销毁同一对象，计数必须正确（TSan 下应干净）。
    //   注意：被指向的对象本身不是线程安全的，这里只并发操作【智能指针】。
    auto sp = MakeShared<int>(7);
    std::vector<std::thread> threads;
    std::atomic<long> ok{0};
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&sp, &ok] {
            for (int j = 0; j < 20000; ++j) {
                SharedPtr<int> copy = sp;   // 并发 +1
                if (*copy == 7) ok.fetch_add(1, std::memory_order_relaxed);
            }                              // 并发 -1
        });
    }
    for (auto& t : threads) t.join();
    EXPECT_EQ(sp.use_count(), 1);
    EXPECT_EQ(ok.load(), 8 * 20000);
}

TEST(SharedPtr, ResetAndBool) {
    SharedPtr<int> sp = MakeShared<int>(3);
    EXPECT_TRUE(static_cast<bool>(sp));
    sp.reset();
    EXPECT_FALSE(static_cast<bool>(sp));
    EXPECT_EQ(sp.use_count(), 0);
}

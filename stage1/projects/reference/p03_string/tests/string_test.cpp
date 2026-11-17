// 项目 3 参考测试：String（SSO / 别名安全 / 差分）
#include <gtest/gtest.h>

#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "common/new_counter.hpp"
#include "string.hpp"

namespace {
bool eq(const String& s, std::string_view sv) { return s.view() == sv; }
}

TEST(String, BasicConstructionAndNulTermination) {
    String a("hello");
    EXPECT_EQ(a.size(), 5u);
    EXPECT_TRUE(eq(a, "hello"));
    EXPECT_EQ(a.c_str()[5], '\0');    // 始终 NUL 结尾

    String b(3, 'x');                 // "xxx"
    EXPECT_TRUE(eq(b, "xxx"));

    String c(std::string_view("world"));
    EXPECT_TRUE(eq(c, "world"));

    String d = a;                     // 拷贝
    EXPECT_TRUE(eq(d, "hello"));
    d.push_back('!');
    EXPECT_TRUE(eq(a, "hello"));      // 深拷贝：改 d 不影响 a
    EXPECT_TRUE(eq(d, "hello!"));
}

TEST(String, LayoutIsReasonable) {
    // 内功核查点：sizeof 合理（16 字节 union + 2×8 计数 = 32）
    static_assert(sizeof(String) == 32, "SSO 布局应为 32 字节");
    SUCCEED();
}

TEST(String, SmallStringZeroHeapAllocation) {
    NewCounter::reset();
    String s("hello");                  // 5 字节 → 短串
    EXPECT_EQ(NewCounter::allocs(), 0);
    EXPECT_FALSE(s.is_long());

    String s15("123456789012345");      // 恰好 15 字节 → 仍短串
    EXPECT_EQ(NewCounter::allocs(), 0);
    EXPECT_FALSE(s15.is_long());

    String s16("1234567890123456");     // 16 字节 → 转长串，必然分配
    EXPECT_EQ(NewCounter::allocs(), 1);
    EXPECT_TRUE(s16.is_long());
}

TEST(String, MoveOfLongStringIsNoAllocAndNoexcept) {
    static_assert(std::is_nothrow_move_constructible_v<String>);
    String big("a string definitely longer than fifteen bytes");
    ASSERT_TRUE(big.is_long());

    NewCounter::reset();
    String moved(std::move(big));       // 长串移动 = 偷指针，O(1) 分配为 0
    EXPECT_EQ(NewCounter::allocs(), 0);
    EXPECT_TRUE(big.empty());           // 源被置空
    EXPECT_TRUE(eq(moved, "a string definitely longer than fifteen bytes"));
}

TEST(String, AliasingSafety) {
    String s("abc");
    s += s;                             // s += s
    EXPECT_TRUE(eq(s, "abcabc"));

    s.append(s.c_str());               // append 自己的 c_str()
    EXPECT_TRUE(eq(s, "abcabcabcabc"));

    String t("xy");
    t.insert(0, t);                     // insert(0, 自己)
    EXPECT_TRUE(eq(t, "xyxy"));

    s.insert(s.size(), s);              // 从尾部插入自己
    EXPECT_TRUE(eq(s, "abcabcabcabcabcabcabcabc"));
}

TEST(String, EmbeddedNulCharacters) {
    String s("a\0b", 3);               // 显式长度构造，含 NUL
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s[1], '\0');
    EXPECT_EQ(s.c_str()[3], '\0');
    EXPECT_TRUE(s.view() == std::string_view("a\0b", 3));
    EXPECT_EQ(s.find('b'), 2u);        // 查找能定位到 NUL 之后的字符
}

TEST(String, FindSubstrStartsEnds) {
    String s("hello world");
    EXPECT_EQ(s.find('o'), 4u);
    EXPECT_EQ(s.find(std::string_view("world")), 6u);
    EXPECT_EQ(s.find("nope"), std::string_view::npos);
    EXPECT_TRUE(eq(s.substr(6), "world"));
    EXPECT_TRUE(eq(s.substr(0, 5), "hello"));
    EXPECT_TRUE(s.starts_with("hello"));
    EXPECT_TRUE(s.ends_with("world"));
    EXPECT_FALSE(s.starts_with("world"));
}

TEST(String, Comparisons) {
    String a("abc"), b("abd"), c("abc");
    EXPECT_TRUE(a == c);
    EXPECT_TRUE(a != b);
    EXPECT_TRUE(a < b);                 // <=> 提供全部比较
    EXPECT_TRUE(b > a);
    EXPECT_TRUE(String("abc") + String("def") == String("abcdef"));
}

TEST(String, AtThrowsOutOfRange) {
    String s("abc");
    EXPECT_EQ(s[0], 'a');               // operator[] 不检查（越界是 UB）
    EXPECT_EQ(s.at(2), 'c');            // at 检查
    EXPECT_THROW(s.at(3), std::out_of_range);
}

TEST(String, ReserveGrowsButNeverShrinks) {
    String s("short");
    const std::size_t cap0 = s.capacity();
    s.reserve(1);                        // 小于当前容量 → 不动
    EXPECT_EQ(s.capacity(), cap0);
    s.reserve(100);                     // 变大
    EXPECT_GE(s.capacity(), 100u);
    const std::size_t cap1 = s.capacity();
    s.reserve(50);                       // 仍不小于 → 不动
    EXPECT_EQ(s.capacity(), cap1);
    EXPECT_TRUE(eq(s, "short"));         // 内容不受影响
}

TEST(String, ClearKeepsCapacity) {
    String s("a long enough string to force heap storage");
    ASSERT_TRUE(s.is_long());
    const std::size_t cap = s.capacity();
    s.clear();
    EXPECT_EQ(s.size(), 0u);
    EXPECT_EQ(s.capacity(), cap);        // clear 不释放容量
    EXPECT_EQ(s.c_str()[0], '\0');
}

TEST(String, ResizeFillsAndTruncates) {
    String s("abc");
    s.resize(5, 'x');
    EXPECT_TRUE(eq(s, "abcxx"));
    s.resize(2);
    EXPECT_TRUE(eq(s, "ab"));
    EXPECT_EQ(s.c_str()[2], '\0');
}

TEST(String, EraseAndInsertMiddle) {
    String s("abcdef");
    s.erase(1, 2);                       // 删 "bc" → "adef"
    EXPECT_TRUE(eq(s, "adef"));
    s.insert(1, "XY");                   // → "aXYdef"
    EXPECT_TRUE(eq(s, "aXYdef"));
    s.insert(s.size(), "Z");             // 尾插
    EXPECT_TRUE(eq(s, "aXYdefZ"));
}

TEST(String, StreamOutput) {
    String s("stream me");
    std::ostringstream os;
    os << s;
    EXPECT_EQ(os.str(), "stream me");
}

// ---------- 差分测试：与 std::string 随机操作 10 万步 ----------
TEST(String, DifferentialAgainstStdString) {
    std::mt19937 rng(20261011);
    String a;
    std::string b;

    auto rand_letter = [&] { return static_cast<char>('a' + rng() % 26); };

    for (int step = 0; step < 100000; ++step) {
        switch (rng() % 7) {
            case 0: {
                char ch = rand_letter();     // 两边必须压入【同一个】字符，否则差分无意义
                a.push_back(ch);
                b.push_back(ch);
                break;
            }
            case 1: {
                std::string add;
                for (int k = static_cast<int>(rng() % 8); k > 0; --k) add.push_back(rand_letter());
                a.append(add);
                b.append(add);
                break;
            }
            case 2:
                if (!b.empty()) {
                    std::size_t pos = rng() % (b.size() + 1);
                    std::size_t cnt = rng() % 5;
                    a.erase(pos, cnt);
                    b.erase(pos, cnt);
                }
                break;
            case 3: {
                std::size_t pos = b.empty() ? 0 : rng() % (b.size() + 1);
                std::string ins;
                for (int k = static_cast<int>(rng() % 4); k > 0; --k) ins.push_back(rand_letter());
                a.insert(pos, ins);
                b.insert(pos, ins);
                break;
            }
            case 4:
                if (!b.empty()) {
                    std::size_t n = rng() % b.size();
                    a.resize(n);
                    b.resize(n);
                }
                break;
            case 5:
                a.reserve(a.size() + (rng() % 16));   // 只影响容量，不改变内容
                break;
            case 6:
                if (rng() % 20 == 0) { a.clear(); b.clear(); }
                break;
        }

        if (b.size() > 512) { a.clear(); b.clear(); }   // 控制规模，避免重复过大

        ASSERT_EQ(a.size(), b.size()) << "step " << step;
        ASSERT_EQ(a.c_str()[a.size()], '\0') << "step " << step;
        ASSERT_TRUE(a.view() == std::string_view(b.data(), b.size())) << "step " << step;
    }
}

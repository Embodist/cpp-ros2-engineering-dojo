#pragma once
// String（含 SSO 小字符串优化）（项目 3）
// ============================================================================
// 训练点：五法则、深拷贝与移动、SSO、别名安全、与 string_view 互操作。
//
// SSO 核心思想：短串（≤15 字节）直接把字符存在对象内部的缓冲区里，不做堆分配；
// 长串才去堆上要一块。这样绝大多数短串（标识符、小配置值）零分配、且缓存友好。
//
// 布局（sizeof(String) == 32）：
//   union { char* heap_; char buf_[16]; }  // 16 字节：长串存指针 / 短串存 15 字符 + NUL
//   std::size_t size_;                     // 8 字节：字符数（不含 NUL）
//   std::size_t capacity_;                 // 8 字节：长串为堆容量；短串固定 = kSSO
// 用 capacity_ > kSSO 区分两种模式（短串 capacity_ 恒等于 kSSO）。
// ============================================================================

#include <compare>
#include <cstddef>
#include <iosfwd>
#include <string_view>
#include <utility>

class String {
public:
    static constexpr std::size_t kSSO = 15;   // 短串最多存 15 个字符（buf_ 16 含 NUL）

    // ---------- 构造 / 赋值 / 析构 ----------
    String() noexcept;
    String(const char* s);                        // 从 C 字符串
    String(const char* s, std::size_t n);         // 从 (指针, 长度)，可含 NUL
    String(std::size_t n, char ch);               // n 个 ch
    // 易错点: 从 string_view 构造用 explicit，避免 string_view 到处隐式变 String（意外分配）。
    explicit String(std::string_view sv);
    String(const String& other);                  // 深拷贝
    String(String&& other) noexcept;              // 移动：长串偷指针，短串实质是拷贝
    ~String();

    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;
    String& operator=(const char* s);
    String& operator=(std::string_view sv);

    void swap(String& other) noexcept;

    // ---------- 访问 ----------
    char&       operator[](std::size_t i) noexcept       { return data()[i]; }
    const char& operator[](std::size_t i) const noexcept { return data()[i]; }
    char&       at(std::size_t i);                       // 越界抛 std::out_of_range
    const char& at(std::size_t i) const;

    char&       front() noexcept       { return data()[0]; }
    const char& front() const noexcept { return data()[0]; }
    char&       back() noexcept        { return data()[size_ - 1]; }
    const char& back() const noexcept  { return data()[size_ - 1]; }

    const char* data() const noexcept  { return is_long() ? heap_ : buf_; }
    char*       data() noexcept        { return is_long() ? heap_ : buf_; }
    const char* c_str() const noexcept { return data(); }   // 始终以 NUL 结尾

    std::size_t size() const noexcept     { return size_; }
    std::size_t length() const noexcept   { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept           { return size_ == 0; }

    bool is_long() const noexcept { return capacity_ > kSSO; }  // 模式标志

    // ---------- 修改 ----------
    void push_back(char ch);
    String& append(std::string_view sv);
    String& append(const char* s);
    String& operator+=(std::string_view sv) { return append(sv); }
    String& operator+=(char ch) { push_back(ch); return *this; }
    String& operator+=(const String& s) { return append(s.view()); }   // 别名安全：s += s

    void insert(std::size_t pos, std::string_view sv);
    void insert(std::size_t pos, char ch) { insert(pos, std::string_view(&ch, 1)); }
    void erase(std::size_t pos, std::size_t count = std::string_view::npos);
    void clear() noexcept;                 // 只置空，不释放 capacity
    void reserve(std::size_t new_cap);     // 只增不减
    void resize(std::size_t n, char fill = '\0');

    // ---------- 查找 / 切片 ----------
    std::size_t find(char ch, std::size_t pos = 0) const noexcept;
    std::size_t find(std::string_view sv, std::size_t pos = 0) const noexcept;
    String substr(std::size_t pos, std::size_t count = std::string_view::npos) const;
    bool starts_with(std::string_view sv) const noexcept;
    bool ends_with(std::string_view sv) const noexcept;

    // ---------- 比较 / 转换 ----------
    std::string_view view() const noexcept { return std::string_view(data(), size_); }
    operator std::string_view() const noexcept { return view(); }   // 只读视图，零拷贝

    friend bool operator==(const String& a, const String& b) noexcept {
        return a.view() == b.view();
    }
    friend std::strong_ordering operator<=>(const String& a, const String& b) noexcept {
        return a.view() <=> b.view();
    }

private:
    // 保证 capacity >= need；可能重新分配（长串）。强异常保证：分配失败时 *this 不变。
    void grow_to(std::size_t need);
    // 把 *this 置为空的短串状态
    void set_short_empty() noexcept {
        buf_[0] = '\0';
        size_ = 0;
        capacity_ = kSSO;
    }

    union {
        char* heap_;         // 长串：堆指针
        char  buf_[kSSO + 1];  // 短串：内联缓冲（含 NUL）
    };
    std::size_t size_     = 0;
    std::size_t capacity_ = kSSO;
};

// 非成员
String operator+(const String& a, const String& b);
String operator+(const String& a, const char* b);
String operator+(const char* a, const String& b);
std::ostream& operator<<(std::ostream& os, const String& s);

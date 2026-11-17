#include "string.hpp"

#include <cstring>
#include <ostream>
#include <stdexcept>

namespace {
constexpr std::size_t npos = std::string_view::npos;
}

// ============================ 构造 / 析构 ============================
String::String() noexcept { set_short_empty(); }

String::String(const char* s) {
    if (s == nullptr) { set_short_empty(); return; }
    const std::size_t n = std::strlen(s);
    if (n <= kSSO) {
        std::memcpy(buf_, s, n);
        buf_[n] = '\0';
        size_ = n;
        capacity_ = kSSO;
    } else {
        heap_ = new char[n + 1];
        std::memcpy(heap_, s, n);
        heap_[n] = '\0';
        size_ = n;
        capacity_ = n;
    }
}

String::String(const char* s, std::size_t n) {
    if (n <= kSSO) {
        std::memcpy(buf_, s, n);
        buf_[n] = '\0';
        size_ = n;
        capacity_ = kSSO;
    } else {
        heap_ = new char[n + 1];
        std::memcpy(heap_, s, n);
        heap_[n] = '\0';
        size_ = n;
        capacity_ = n;
    }
}

String::String(std::size_t n, char ch) {
    if (n <= kSSO) {
        std::memset(buf_, ch, n);
        buf_[n] = '\0';
        size_ = n;
        capacity_ = kSSO;
    } else {
        heap_ = new char[n + 1];
        std::memset(heap_, ch, n);
        heap_[n] = '\0';
        size_ = n;
        capacity_ = n;
    }
}

String::String(std::string_view sv) {
    const std::size_t n = sv.size();
    if (n <= kSSO) {
        std::memcpy(buf_, sv.data(), n);
        buf_[n] = '\0';
        size_ = n;
        capacity_ = kSSO;
    } else {
        heap_ = new char[n + 1];
        std::memcpy(heap_, sv.data(), n);
        heap_[n] = '\0';
        size_ = n;
        capacity_ = n;
    }
}

// 深拷贝：长串复制堆内容，短串复制内联缓冲
String::String(const String& o) : size_(o.size_), capacity_(o.capacity_) {
    if (o.is_long()) {
        heap_ = new char[o.capacity_ + 1];
        std::memcpy(heap_, o.heap_, o.size_ + 1);
    } else {
        std::memcpy(buf_, o.buf_, o.size_ + 1);
    }
}

// 移动：长串直接偷指针（O(1)、不分配）；短串没有指针可偷，只能整块拷贝
String::String(String&& o) noexcept : size_(o.size_), capacity_(o.capacity_) {
    if (o.is_long()) {
        heap_ = o.heap_;         // 偷走堆指针
    } else {
        std::memcpy(buf_, o.buf_, o.size_ + 1);  // 短串移动 ≈ 拷贝
    }
    o.set_short_empty();          // 源一律变成空短串
}

String::~String() {
    if (is_long()) delete[] heap_;
}

// ============================ 赋值 ============================
String& String::operator=(const String& o) {
    if (this != &o) {
        String tmp(o);   // copy-and-swap：先构造成功的副本，再一次性换入
        swap(tmp);       // tmp 析构时释放旧资源；若 tmp 构造失败，*this 不变（强保证）
    }
    return *this;
}

String& String::operator=(String&& o) noexcept {
    if (this == &o) return *this;
    if (is_long()) delete[] heap_;   // 用当前（未被覆盖的）capacity_ 判断
    size_ = o.size_;
    capacity_ = o.capacity_;
    if (o.is_long()) {
        heap_ = o.heap_;
    } else {
        std::memcpy(buf_, o.buf_, o.size_ + 1);
    }
    o.set_short_empty();
    return *this;
}

String& String::operator=(const char* s) {
    String tmp(s);
    swap(tmp);
    return *this;
}

String& String::operator=(std::string_view sv) {
    String tmp(sv);
    swap(tmp);
    return *this;
}

void String::swap(String& o) noexcept {
    if (this == &o) return;
    std::swap(size_, o.size_);
    std::swap(capacity_, o.capacity_);
    // 交换 union：16 字节整体搬。两种模式各自带着自己的 (buffer, size, capacity) 三元组，
    // 整体交换后仍各自自洽。
    char tmp[sizeof(buf_)];
    std::memcpy(tmp, buf_, sizeof(buf_));
    std::memcpy(buf_, o.buf_, sizeof(buf_));
    std::memcpy(o.buf_, tmp, sizeof(buf_));
}

// ============================ 访问 ============================
char& String::at(std::size_t i) {
    if (i >= size_) throw std::out_of_range("String::at");
    return data()[i];
}

const char& String::at(std::size_t i) const {
    if (i >= size_) throw std::out_of_range("String::at");
    return data()[i];
}

// ============================ 修改 ============================
// grow_to：needed 字节放不下时重新分配。强异常保证：先分配成功、再搬、再释放旧块。
void String::grow_to(std::size_t need) {
    if (need <= capacity_) return;
    std::size_t new_cap = capacity_;
    // 取舍: 增长因子约 1.5（new + new/2 + 1）。1.5 相比 2 更省内存、也不易触发"内存复用"，
    //       代价是可能更频繁重分配。标准库实现多为 2。
    while (new_cap < need) new_cap = new_cap + new_cap / 2 + 1;
    if (new_cap <= kSSO) new_cap = kSSO + 1;   // 长串容量至少 16，离开短串区

    char* new_buf = new char[new_cap + 1];     // 可能抛 bad_alloc → 此时 *this 完全未动
    std::memcpy(new_buf, data(), size_ + 1);
    if (is_long()) delete[] heap_;
    heap_ = new_buf;
    capacity_ = new_cap;
}

void String::push_back(char ch) {
    if (size_ + 1 > capacity_) grow_to(size_ + 1);
    char* d = data();
    d[size_] = ch;
    d[size_ + 1] = '\0';
    ++size_;
}

// append：处理别名（源可能就指向 *this 的存储）
String& String::append(std::string_view sv) {
    const std::size_t n = sv.size();
    if (n == 0) return *this;

    const char* base = data();
    const char* src = sv.data();
    std::size_t offset = npos;
    // 易错点: s.append(s.c_str()) / s += s 时，src 就在自己的缓冲区里。
    //         一旦扩容重分配，src 立刻悬空。所以先记下偏移，扩容后重新计算地址。
    if (src >= base && src <= base + size_) offset = static_cast<std::size_t>(src - base);

    if (size_ + n > capacity_) grow_to(size_ + n);
    if (offset != npos) src = data() + offset;

    // 用 memmove：即使源与目标区间重叠也安全
    std::memmove(data() + size_, src, n);
    size_ += n;
    data()[size_] = '\0';
    return *this;
}

String& String::append(const char* s) { return append(std::string_view(s)); }

void String::insert(std::size_t pos, std::string_view sv) {
    if (pos > size_) throw std::out_of_range("String::insert");
    const std::size_t n = sv.size();
    if (n == 0) return;

    const char* base = data();
    const char* src = sv.data();
    std::size_t offset = npos;
    if (src >= base && src <= base + size_) offset = static_cast<std::size_t>(src - base);

    if (size_ + n > capacity_) grow_to(size_ + n);
    if (offset != npos) src = data() + offset;

    char* d = data();
    std::memmove(d + pos + n, d + pos, size_ - pos);  // 先把尾部右移 n
    std::memmove(d + pos, src, n);                    // 再写入新内容
    size_ += n;
    d[size_] = '\0';
}

void String::erase(std::size_t pos, std::size_t count) {
    if (pos > size_) throw std::out_of_range("String::erase");
    if (count == npos || pos + count >= size_) count = size_ - pos;
    char* d = data();
    std::memmove(d + pos, d + pos + count, size_ - pos - count);
    size_ -= count;
    d[size_] = '\0';
}

// 易错点: clear 只把长度置 0，**不释放** capacity（与标准库一致）——这样再次增长可复用内存。
void String::clear() noexcept {
    size_ = 0;
    data()[0] = '\0';
}

void String::reserve(std::size_t new_cap) {
    if (new_cap > capacity_) grow_to(new_cap);   // reserve 只增不减
}

void String::resize(std::size_t n, char fill) {
    if (n < size_) {
        size_ = n;
        data()[n] = '\0';
    } else if (n > size_) {
        if (n > capacity_) grow_to(n);
        char* d = data();
        std::memset(d + size_, fill, n - size_);
        size_ = n;
        d[n] = '\0';
    }
}

// ============================ 查找 / 切片 ============================
std::size_t String::find(char ch, std::size_t pos) const noexcept {
    if (pos > size_) return npos;
    const void* r = std::memchr(data() + pos, ch, size_ - pos);
    return r ? static_cast<std::size_t>(static_cast<const char*>(r) - data()) : npos;
}

// 直接借力 string_view 的查找（string_view 可含 NUL，语义正确）
std::size_t String::find(std::string_view sv, std::size_t pos) const noexcept {
    return view().find(sv, pos);
}

String String::substr(std::size_t pos, std::size_t count) const {
    return String(view().substr(pos, count));
}

bool String::starts_with(std::string_view sv) const noexcept { return view().starts_with(sv); }
bool String::ends_with(std::string_view sv) const noexcept   { return view().ends_with(sv); }

// ============================ 非成员 ============================
String operator+(const String& a, const String& b) {
    String r;
    r.reserve(a.size() + b.size());   // 预留足够容量 → 只分配一次
    r.append(a.view());
    r.append(b.view());
    return r;
}

String operator+(const String& a, const char* b) { return a + String(b); }
String operator+(const char* a, const String& b) { return String(a) + b; }

std::ostream& operator<<(std::ostream& os, const String& s) {
    os.write(s.data(), static_cast<std::streamsize>(s.size()));
    return os;
}

#pragma once
// UniquePtr / DefaultDelete / MakeUnique（项目 2a）
// ============================================================================
// 训练点：RAII、独占所有权、五法则、纯移动类型、EBO（空基类优化）/ no_unique_address。
//
// 核心想法：UniquePtr 只允许移动不允许拷贝 —— 编译期就杜绝"两个指针拥有同一对象"。
// 易错点: 拷贝构造函数必须 = delete，否则浅拷贝会造成 double free。
// ============================================================================

#include <cstddef>
#include <type_traits>
#include <utility>

// ---------------- 默认删除器 ----------------
template <class T>
struct DefaultDelete {
    constexpr DefaultDelete() noexcept = default;

    // 转换构造：DefaultDelete<U> → DefaultDelete<T>（允许 U* 隐式转 T*，如派生→基类）
    template <class U, class = std::enable_if_t<std::is_convertible_v<U*, T*>>>
    constexpr DefaultDelete(const DefaultDelete<U>&) noexcept {}

    void operator()(T* p) const noexcept { delete p; }
};

// 数组特化：删除时用 delete[]
// 参考: 标准 std::default_delete 也是这么分单个对象 / 数组两条路。
template <class T>
struct DefaultDelete<T[]> {
    constexpr DefaultDelete() noexcept = default;
    template <class U, class = std::enable_if_t<std::is_convertible_v<U (*)[], T (*)[]>>>
    constexpr DefaultDelete(const DefaultDelete<U[]>&) noexcept {}
    void operator()(T* p) const noexcept { delete[] p; }
};

// ---------------- UniquePtr（单个对象）----------------
template <class T, class D = DefaultDelete<T>>
class UniquePtr {
public:
    using pointer      = T*;
    using element_type = T;
    using deleter_type = D;

    constexpr UniquePtr() noexcept : ptr_(nullptr), deleter_{} {}
    constexpr UniquePtr(std::nullptr_t) noexcept : ptr_(nullptr) {}
    explicit UniquePtr(pointer p) noexcept : ptr_(p) {}
    UniquePtr(pointer p, D d) noexcept : ptr_(p), deleter_(std::move(d)) {}

    // 移动：把资源"搬"走，源置空 —— 所有权始终唯一
    UniquePtr(UniquePtr&& o) noexcept
        : ptr_(o.release()), deleter_(std::move(o.deleter_)) {}

    // 派生类 → 基类（或 deleter 可转换）的移动构造，用 concepts 风格约束
    template <class U, class E,
              class = std::enable_if_t<std::is_convertible_v<U*, T*> &&
                                       std::is_constructible_v<D, E&&>>>
    UniquePtr(UniquePtr<U, E>&& o) noexcept
        : ptr_(o.release()), deleter_(std::move(o.get_deleter())) {}

    // 易错点: 拷贝必须删除，否则 UniquePtr 语义（独占）被破坏，且会 double free。
    UniquePtr(const UniquePtr&)            = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) {
            // 易错点: 自移动赋值 p = std::move(p) 若先 reset 会把 p 自己的资源删掉。
            //         所以先判 self，再 reset(o.release())——release 先取出指针，
            //         即使后面 reset 也安全。
            reset(o.release());
            deleter_ = std::move(o.deleter_);
        }
        return *this;
    }

    ~UniquePtr() { if (ptr_) deleter_(ptr_); }

    T*  get()     const noexcept { return ptr_; }
    T*  release()       noexcept { T* p = ptr_; ptr_ = nullptr; return p; }  // 交出所有权，不删

    void reset(T* p = nullptr) noexcept {
        // 易错点: reset 的经典意外——`up.reset(up.get())` 会先删旧对象再接管同一指针 → use-after-free。
        //         标准库对此行为是"未定义"，一般要求调用者避免；这里保持与标准一致。
        if (ptr_) deleter_(ptr_);
        ptr_ = p;
    }

    void swap(UniquePtr& o) noexcept { std::swap(ptr_, o.ptr_); std::swap(deleter_, o.deleter_); }

    T& operator*()  const { return *ptr_; }
    T* operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    const D& get_deleter() const noexcept { return deleter_; }
    D&       get_deleter()       noexcept { return deleter_; }

private:
    // 说明: 成员声明顺序 = 初始化顺序。ptr_ 在前、deleter_ 在后，构造函数初始化列表
    //       也按此顺序写，避免 -Wreorder。
    T* ptr_ = nullptr;
    // 易错点: [[no_unique_address]]（C++20）让【空】deleter 不占用任何存储，
    //         于是 sizeof(UniquePtr<int>) == sizeof(int*)。libstdc++ 用 EBO 继承实现同一目标。
    [[no_unique_address]] D deleter_{};
};

// ---------------- UniquePtr<T[]>（数组特化）----------------
template <class T, class D>
class UniquePtr<T[], D> {
public:
    using pointer      = T*;
    using element_type = T;
    using deleter_type = D;

    constexpr UniquePtr() noexcept : ptr_(nullptr), deleter_{} {}
    explicit UniquePtr(pointer p) noexcept : ptr_(p) {}
    UniquePtr(pointer p, D d) noexcept : ptr_(p), deleter_(std::move(d)) {}

    UniquePtr(UniquePtr&& o) noexcept : ptr_(o.release()), deleter_(std::move(o.deleter_)) {}

    UniquePtr(const UniquePtr&)            = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) reset(o.release());
        return *this;
    }

    ~UniquePtr() { if (ptr_) deleter_(ptr_); }

    T& operator[](std::size_t i) const { return ptr_[i]; }   // 数组特化独有
    T*  get()     const noexcept { return ptr_; }
    T*  release()       noexcept { T* p = ptr_; ptr_ = nullptr; return p; }
    void reset(T* p = nullptr) noexcept { if (ptr_) deleter_(ptr_); ptr_ = p; }
    void swap(UniquePtr& o) noexcept { std::swap(ptr_, o.ptr_); std::swap(deleter_, o.deleter_); }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    T* ptr_ = nullptr;
    [[no_unique_address]] D deleter_{};
};

// ---------------- MakeUnique ----------------
// 易错点: 两个重载要互相排斥，否则 MakeUnique<int[]>(5) 会同时匹配，
//         且可变参版本对 int 是"精确匹配"、数组版本要 int→size_t 转换，
//         结果可变参版本胜出 → 编译错误。用 requires 把两者分开。
template <class T, class... Args>
    requires (!std::is_array_v<T>)
UniquePtr<T> MakeUnique(Args&&... args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

// 数组版本：MakeUnique<int[]>(n)
template <class T>
    requires std::is_array_v<T>
UniquePtr<T> MakeUnique(std::size_t n) {
    using U = std::remove_extent_t<T>;
    return UniquePtr<T>(new U[n]());   // () 值初始化，元素清零
}

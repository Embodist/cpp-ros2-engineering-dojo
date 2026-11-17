#pragma once
// Vector<T, Alloc> —— 连续存储动态数组（项目 5，旗舰）
// ============================================================================
// 训练点：原始内存与对象生命周期分离、三指针表示、五法则、强异常保证、
//         move_if_noexcept、别名安全、迭代器类别分发、contiguous_iterator、concepts。
//
// 三指针表示（与 std::vector 同构）：
//   T* begin_  指向第一个元素（也是缓冲区起点）
//   T* end_    指向最后一个元素的【后一位】（size = end_ - begin_）
//   T* cap_    指向缓冲区的【后一位】（capacity = cap_ - begin_）
// 这样 size()/capacity()/empty() 都是 O(1) 指针相减。
//
// 底层存储【不用】unique_ptr<T[]>：那会默认构造 capacity 个元素，与"容量 ≠ 元素数"矛盾。
// 我们用分配器要原始内存，元素用 construct/destroy 逐个管理生命周期。
// ============================================================================

#include <algorithm>
#include <compare>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <class T, class Alloc = std::allocator<T>> class Vector {
public:
    using value_type = T;
    using allocator_type = Alloc;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = typename std::allocator_traits<Alloc>::pointer;
    using const_pointer = typename std::allocator_traits<Alloc>::const_pointer;

    // 迭代器就是原生指针：它天然是 contiguous_iterator（随机访问的超集）。
    using iterator = T*;
    using const_iterator = const T*;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ======================= 构造 / 赋值 / 析构 =======================
    Vector() noexcept = default;
    explicit Vector(const Alloc& a) noexcept : alloc_(a) {}

    explicit Vector(size_type n) : alloc_() { default_append(n); }
    Vector(size_type n, const T& v, const Alloc& a = Alloc()) : alloc_(a) { fill_append(n, v); }
    Vector(size_type n, const Alloc& a) : alloc_(a) { default_append(n); }

    Vector(std::initializer_list<T> il, const Alloc& a = Alloc()) : alloc_(a) {
        reserve(il.size());
        for (const T& v : il)
            construct_at(end_++, v);
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    Vector(It first, It last, const Alloc& a = Alloc()) : alloc_(a) {
        append_range(first, last);
    }

    // 拷贝：新缓冲 + 逐个拷贝构造。中途抛异常则销毁已构造的、释放缓冲（不漏、强保证）。
    Vector(const Vector& o)
        : alloc_(std::allocator_traits<Alloc>::select_on_container_copy_construction(o.alloc_)) {
        if (o.size() != 0) {
            begin_ = allocate(o.size());
            size_type k = 0;
            try {
                for (; k < o.size(); ++k)
                    construct_at(begin_ + k, o.begin_[k]);
            } catch (...) {
                destroy_range(begin_, begin_ + k);
                deallocate(begin_, o.size());
                throw;
            }
            end_ = cap_ = begin_ + o.size();
        }
    }
    Vector(const Vector& o, const Alloc& a) : alloc_(a) {
        if (o.size() != 0) {
            begin_ = allocate(o.size());
            size_type k = 0;
            try {
                for (; k < o.size(); ++k)
                    construct_at(begin_ + k, o.begin_[k]);
            } catch (...) {
                destroy_range(begin_, begin_ + k);
                deallocate(begin_, o.size());
                throw;
            }
            end_ = cap_ = begin_ + o.size();
        }
    }

    // 移动：只搬三个指针（O(1)）。noexcept 让 Vector 自身也能被"高效搬移"。
    Vector(Vector&& o) noexcept
        : alloc_(std::move(o.alloc_)), begin_(o.begin_), end_(o.end_), cap_(o.cap_) {
        o.begin_ = o.end_ = o.cap_ = nullptr;
    }
    Vector(Vector&& o, const Alloc& a) : alloc_(a) {
        if (alloc_ == o.alloc_) {
            begin_ = o.begin_;
            end_ = o.end_;
            cap_ = o.cap_;
            o.begin_ = o.end_ = o.cap_ = nullptr;
        } else {
            for (T& v : o)
                emplace_back(std::move(v));
            o.clear();
        }
    }

    ~Vector() { destroy_all(); }

    // 拷贝赋值：copy-and-swap → 强异常保证
    Vector& operator=(const Vector& o) {
        if (this != &o) {
            Vector tmp(o);
            swap(tmp);
        }
        return *this;
    }
    // 移动赋值：分配器可传播或相等 → O(1) 偷；否则只能逐元素搬
    Vector& operator=(Vector&& o) {
        if (this == &o)
            return *this;
        destroy_all();
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value)
            alloc_ = std::move(o.alloc_);
        if (alloc_ == o.alloc_) {
            begin_ = o.begin_;
            end_ = o.end_;
            cap_ = o.cap_;
            o.begin_ = o.end_ = o.cap_ = nullptr;
        } else {
            reserve(o.size());
            for (T& v : o)
                emplace_back(std::move(v));
            o.clear();
        }
        return *this;
    }
    Vector& operator=(std::initializer_list<T> il) {
        Vector tmp(il, alloc_);
        swap(tmp);
        return *this;
    }

    // ======================= 赋值（用于已有对象）=======================
    void assign(size_type n, const T& v) {
        // 易错点: v 可能就引用本容器里的元素。先拷一份再 clear，否则 source 被销毁。
        T tmp(v);
        clear();
        reserve(n);
        size_type k = 0;
        try {
            for (; k < n; ++k)
                construct_at(begin_ + k, tmp);
        } catch (...) {
            destroy_range(begin_, begin_ + k);
            throw;
        }
        end_ = begin_ + n;
    }
    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    void assign(It first, It last) {
        clear();
        if constexpr (std::forward_iterator<It>)
            reserve(static_cast<size_type>(std::distance(first, last)));
        for (; first != last; ++first)
            emplace_back(*first);
    }
    void assign(std::initializer_list<T> il) { assign(il.begin(), il.end()); }

    // ======================= 容量 =======================
    size_type size() const noexcept { return static_cast<size_type>(end_ - begin_); }
    size_type capacity() const noexcept { return static_cast<size_type>(cap_ - begin_); }
    bool empty() const noexcept { return begin_ == end_; }
    size_type max_size() const noexcept { return std::allocator_traits<Alloc>::max_size(alloc_); }

    void reserve(size_type n) {
        if (n > capacity())
            reallocate_to(n); // reserve 只增不减
    }
    // shrink_to_fit：把容量收缩到恰好 size。可能重分配。
    void shrink_to_fit() {
        if (size() < capacity())
            reallocate_to(size());
    }

    void resize(size_type n) { resize(n, T()); }
    void resize(size_type n, const T& v) {
        if (n < size()) {
            destroy_range(begin_ + n, end_); // 缩小：只析构，不释放 capacity
            end_ = begin_ + n;
        } else if (n > size()) {
            reserve(n);
            size_type k = size();
            try {
                for (; k < n; ++k)
                    construct_at(begin_ + k, v);
            } catch (...) {
                destroy_range(begin_ + size(), begin_ + k);
                throw;
            }
            end_ = begin_ + n;
        }
    }

    // ======================= 访问 =======================
    reference operator[](size_type i) noexcept { return begin_[i]; }
    const_reference operator[](size_type i) const noexcept { return begin_[i]; }
    reference at(size_type i) {
        if (i >= size())
            throw std::out_of_range("Vector::at");
        return begin_[i];
    }
    const_reference at(size_type i) const {
        if (i >= size())
            throw std::out_of_range("Vector::at");
        return begin_[i];
    }
    reference front() noexcept { return begin_[0]; }
    const_reference front() const noexcept { return begin_[0]; }
    reference back() noexcept { return end_[-1]; }
    const_reference back() const noexcept { return end_[-1]; }
    T* data() noexcept { return begin_; }
    const T* data() const noexcept { return begin_; }

    // ======================= 迭代器 =======================
    iterator begin() noexcept { return begin_; }
    iterator end() noexcept { return end_; }
    const_iterator begin() const noexcept { return begin_; }
    const_iterator end() const noexcept { return end_; }
    const_iterator cbegin() const noexcept { return begin_; }
    const_iterator cend() const noexcept { return end_; }
    reverse_iterator rbegin() noexcept { return reverse_iterator(end_); }
    reverse_iterator rend() noexcept { return reverse_iterator(begin_); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end_); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin_); }

    // ======================= 尾部修改 =======================
    template <class... Args> reference emplace_back(Args&&... args) {
        if (end_ != cap_) {
            // 无扩容：直接在未构造的槽上构造。抛异常时 end_ 未变 → 原数据不变（强保证）。
            construct_at(end_, std::forward<Args>(args)...);
            ++end_;
            return back();
        }
        return emplace_back_realloc(std::forward<Args>(args)...);
    }
    void push_back(const T& v) { emplace_back(v); }
    void push_back(T&& v) { emplace_back(std::move(v)); }

    void pop_back() noexcept {
        --end_;
        destroy_at(end_);
    }

    // ======================= 中间插入 =======================
    iterator insert(const_iterator pos, const T& v) {
        // 先做一份独立副本：v 可能就指向本缓冲区，扩容搬移后原址就变了。
        T tmp(v);
        return insert_at(static_cast<size_type>(pos - cbegin()), std::move(tmp));
    }
    iterator insert(const_iterator pos, T&& v) {
        // 只有可拷贝类型才用"先拷一份"的办法规避别名；move-only 类型无法拷贝，
        // 与标准库一样不保证 insert(pos, std::move(自元素)) 这种别名用法。
        if constexpr (std::is_copy_constructible_v<T>) {
            if (aliases(std::addressof(v))) {
                T tmp(v);
                return insert_at(static_cast<size_type>(pos - cbegin()), std::move(tmp));
            }
        }
        return insert_at(static_cast<size_type>(pos - cbegin()), std::move(v));
    }
    template <class... Args> iterator emplace(const_iterator pos, Args&&... args) {
        T tmp(std::forward<Args>(args)...); // 先构造成临时：规避别名，逻辑最清晰
        return insert_at(static_cast<size_type>(pos - cbegin()), std::move(tmp));
    }

    iterator insert(const_iterator pos, size_type n, const T& v) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if (n == 0)
            return begin_ + idx;
        T tmp(v); // 同上：规避 v 引用到本容器元素
        reserve(size() + n);
        size_type old = size();
        size_type k = 0;
        try {
            for (; k < n; ++k)
                construct_at(begin_ + old + k, tmp);
        } catch (...) {
            destroy_range(begin_ + old, begin_ + old + k);
            throw;
        }
        end_ = begin_ + old + n;
        std::rotate(begin_ + idx, begin_ + old, end_); // 把尾部这 n 个旋到 idx
        return begin_ + idx;
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    iterator insert(const_iterator pos, It first, It last) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        if constexpr (std::forward_iterator<It>) {
            size_type n = static_cast<size_type>(std::distance(first, last));
            if (n == 0)
                return begin_ + idx;
            reserve(size() + n);
            size_type old = size();
            size_type k = 0;
            try {
                for (; first != last; ++first, ++k)
                    construct_at(begin_ + old + k, *first);
            } catch (...) {
                destroy_range(begin_ + old, begin_ + old + k);
                throw;
            }
            end_ = begin_ + old + n;
            std::rotate(begin_ + idx, begin_ + old, end_);
        } else {
            // input 迭代器：无法预知长度，逐个插（每次插在同一个位置之后）
            iterator it = begin_ + idx;
            for (; first != last; ++first) {
                it = insert(it, *first);
                ++it;
            }
        }
        return begin_ + idx;
    }
    iterator insert(const_iterator pos, std::initializer_list<T> il) {
        return insert(pos, il.begin(), il.end());
    }

    // ======================= 删除 =======================
    iterator erase(const_iterator pos) {
        size_type idx = static_cast<size_type>(pos - cbegin());
        std::move(begin_ + idx + 1, end_, begin_ + idx); // 左移覆盖
        --end_;
        destroy_at(end_);
        return begin_ + idx;
    }
    iterator erase(const_iterator first, const_iterator last) {
        size_type fi = static_cast<size_type>(first - cbegin());
        size_type li = static_cast<size_type>(last - cbegin());
        size_type n = li - fi;
        if (n == 0)
            return begin_ + fi;
        std::move(begin_ + li, end_, begin_ + fi);
        destroy_range(end_ - n, end_);
        end_ -= n;
        return begin_ + fi;
    }
    void clear() noexcept {
        destroy_range(begin_, end_);
        end_ = begin_;
    } // 不释放 capacity

    void swap(Vector& o) noexcept {
        std::swap(begin_, o.begin_);
        std::swap(end_, o.end_);
        std::swap(cap_, o.cap_);
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value)
            std::swap(alloc_, o.alloc_);
    }

    allocator_type get_allocator() const noexcept { return alloc_; }

    // ======================= 比较 =======================
    // 易错点: 不要把 == / <=> 写成类内 friend 定义——类内 friend 会在【类实例化时】
    //         就实例化其签名与函数体，于是要求 T 必须有 == / <=> 。像 Tracer、ThrowOnN
    //         这类没有比较运算符的元素类型会让整个 Vector 无法实例化。
    //         改写成类外的【函数模板】，只有真正比较时才实例化，要求才生效。
    friend void swap(Vector& a, Vector& b) noexcept { a.swap(b); }

private:
    // 扩容时【新缓冲区】的 RAII 守卫：异常时销毁已构造的、释放缓冲；成功则 release()。
    struct BufferGuard {
        Vector* self;
        T* p;
        size_type cap;
        size_type built = 0;
        bool active = true;
        ~BufferGuard() {
            if (active) {
                self->destroy_range(p, p + built);
                self->deallocate(p, cap);
            }
        }
        void release() noexcept { active = false; }
    };

    // ---------- 分配 / 构造 / 析构 ----------
    T* allocate(size_type n) { return std::allocator_traits<Alloc>::allocate(alloc_, n); }
    void deallocate(T* p, size_type n) noexcept {
        std::allocator_traits<Alloc>::deallocate(alloc_, p, n);
    }

    template <class... Args> void construct_at(T* p, Args&&... a) {
        std::allocator_traits<Alloc>::construct(alloc_, p, std::forward<Args>(a)...);
    }
    void destroy_at(T* p) noexcept { std::allocator_traits<Alloc>::destroy(alloc_, p); }
    void destroy_range(T* f, T* l) noexcept {
        for (; f != l; ++f)
            destroy_at(f);
    }
    void destroy_all() noexcept {
        destroy_range(begin_, end_);
        deallocate(begin_, capacity());
        begin_ = end_ = cap_ = nullptr;
    }

    bool aliases(const T* p) const noexcept { return begin_ != nullptr && begin_ <= p && p < end_; }

    // 取舍: 增长因子 1.5（= cap + cap/2）。相比 2 更省内存、更易复用刚释放的块；
    //       代价是重分配更频繁。摊还后 n 次 push_back 仍是 O(n)。
    size_type grow_capacity(size_type need) const {
        size_type c = capacity();
        size_type grown = c ? c + c / 2 : 1;
        return grown < need ? need : grown;
    }

    // 重分配到 new_cap：用 move_if_noexcept（能安全移动就移动，否则拷贝）。
    void reallocate_to(size_type new_cap) {
        if (new_cap == 0) {
            destroy_all();
            return;
        }
        T* nd = allocate(new_cap);
        BufferGuard g{this, nd, new_cap};
        for (; g.built < size(); ++g.built)
            construct_at(nd + g.built, std::move_if_noexcept(begin_[g.built]));
        g.release();
        size_type n = size();
        destroy_range(begin_, end_);
        deallocate(begin_, capacity());
        begin_ = nd;
        end_ = nd + n;
        cap_ = nd + new_cap;
    }

    // 扩容 + 尾插。易错点: 必须【先构造新元素、再搬旧元素】——新元素的实参可能
    //         引用旧缓冲里的元素（push_back(v[0])）；若先搬（尤其是移动搬移会把源置空），
    //         实参就变成 moved-from 了。旧缓冲此刻还完好，先读它才安全。
    template <class... Args> reference emplace_back_realloc(Args&&... args) {
        size_type new_cap = grow_capacity(size() + 1);
        T* nd = allocate(new_cap);
        bool val_done = false;
        size_type k = 0;
        try {
            construct_at(nd + size(), std::forward<Args>(args)...); // 1) 先建新元素（别名安全）
            val_done = true;
            for (; k < size(); ++k) // 2) 再搬旧元素
                construct_at(nd + k, std::move_if_noexcept(begin_[k]));
        } catch (...) {
            if (val_done)
                destroy_at(nd + size());
            destroy_range(nd, nd + k);
            deallocate(nd, new_cap);
            throw;
        }
        size_type n = size();
        destroy_range(begin_, end_);
        deallocate(begin_, capacity());
        begin_ = nd;
        end_ = nd + n + 1;
        cap_ = nd + new_cap;
        return nd[n];
    }

    // 在 idx 处插入一个【非别名】的右值 value。value 来自栈上临时对象，搬家不影响它。
    iterator insert_at(size_type idx, T&& value) {
        if (idx == size()) { // 等价 push_back
            if (end_ == cap_) {
                emplace_back_realloc(std::move(value));
                return begin_ + idx;
            }
            construct_at(end_, std::move(value));
            ++end_;
            return begin_ + idx;
        }
        if (end_ == cap_)
            return insert_realloc(idx, std::move(value));

        // 无扩容：先把末尾元素搬到新槽（占位），再整体右移，最后把 value 赋到 idx。
        size_type n = size();
        construct_at(end_, std::move_if_noexcept(begin_[n - 1]));
        ++end_;
        for (size_type i = n - 1; i > idx; --i)
            begin_[i] = std::move(begin_[i - 1]);
        begin_[idx] = std::move(value);
        return begin_ + idx;
    }

    iterator insert_realloc(size_type idx, T&& value) {
        size_type new_cap = grow_capacity(size() + 1);
        T* nd = allocate(new_cap);
        size_type k = 0;
        try {
            for (; k < idx; ++k)
                construct_at(nd + k, std::move_if_noexcept(begin_[k]));
            construct_at(nd + k, std::move(value));
            ++k; // 新元素
            for (size_type j = idx; j < size(); ++j, ++k)
                construct_at(nd + k, std::move_if_noexcept(begin_[j]));
        } catch (...) {
            destroy_range(nd, nd + k); // 已构造的是 [0, k) 连续前缀
            deallocate(nd, new_cap);
            throw;
        }
        size_type n = size();
        destroy_range(begin_, end_);
        deallocate(begin_, capacity());
        begin_ = nd;
        end_ = nd + n + 1;
        cap_ = nd + new_cap;
        return begin_ + idx;
    }

    void default_append(size_type n) {
        if (n == 0)
            return;
        begin_ = allocate(n);
        size_type k = 0;
        try {
            for (; k < n; ++k)
                construct_at(begin_ + k);
        } catch (...) {
            destroy_range(begin_, begin_ + k);
            deallocate(begin_, n);
            throw;
        }
        end_ = cap_ = begin_ + n;
    }
    void fill_append(size_type n, const T& v) {
        if (n == 0)
            return;
        begin_ = allocate(n);
        size_type k = 0;
        try {
            for (; k < n; ++k)
                construct_at(begin_ + k, v);
        } catch (...) {
            destroy_range(begin_, begin_ + k);
            deallocate(begin_, n);
            throw;
        }
        end_ = cap_ = begin_ + n;
    }
    template <class It> void append_range(It first, It last) {
        if constexpr (std::forward_iterator<It>)
            reserve(static_cast<size_type>(std::distance(first, last)));
        for (; first != last; ++first)
            emplace_back(*first);
    }

    [[no_unique_address]] Alloc alloc_{};
    T* begin_ = nullptr;
    T* end_ = nullptr;
    T* cap_ = nullptr;
};

// 类外比较：仅在真正被调用时实例化，从而不强制所有元素类型都支持 == / <=>。
template <class T, class A>
    requires std::equality_comparable<T>
bool operator==(const Vector<T, A>& a, const Vector<T, A>& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
template <class T, class A>
    requires std::three_way_comparable<T>
auto operator<=>(const Vector<T, A>& a, const Vector<T, A>& b) {
    return std::lexicographical_compare_three_way(a.begin(), a.end(), b.begin(), b.end(),
                                                  std::compare_three_way());
}

// 非成员 erase / erase_if（C++20 的 std::erase 风格）
template <class T, class A, class U>
typename Vector<T, A>::size_type erase(Vector<T, A>& c, const U& value) {
    T* new_last = std::remove(c.begin(), c.end(), value);
    auto n = static_cast<typename Vector<T, A>::size_type>(c.end() - new_last);
    c.erase(new_last, c.end());
    return n;
}
template <class T, class A, class Pred>
typename Vector<T, A>::size_type erase_if(Vector<T, A>& c, Pred pred) {
    T* new_last = std::remove_if(c.begin(), c.end(), pred);
    auto n = static_cast<typename Vector<T, A>::size_type>(c.end() - new_last);
    c.erase(new_last, c.end());
    return n;
}

// C++20 concepts 核查点
static_assert(std::contiguous_iterator<Vector<int>::iterator>);
static_assert(std::contiguous_iterator<Vector<int>::const_iterator>);

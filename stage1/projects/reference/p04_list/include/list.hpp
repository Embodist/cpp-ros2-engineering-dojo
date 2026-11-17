#pragma once
// List<T, Alloc> —— 带哨兵节点的双向循环链表（项目 4）
// ============================================================================
// 训练点：节点所有权、自定义迭代器（双向）、迭代器失效规则、哨兵节点、
//         分配器感知、异常安全、模板与 concepts、稳定归并排序。
//
// 三个贯穿全篇的设计决定：
//   1) 哨兵（sentinel）：一个不存值的"假节点"常驻链表里，首尾都指向它。
//      它把"空表""表头插入""表尾插入"统一成同一种链接操作 → 省掉大量 if 特判。
//   2) 节点用【重绑定的分配器】分配，元素用【原分配器】构造。
//      allocator_traits 管一切构造/析构/分配/释放，兼容有状态分配器。
//   3) 析构走【迭代循环】而非递归 / unique_ptr 链。后者在 10 万节点时会
//      递归 10 万层直接爆栈（见 clear() 处注释）。
// ============================================================================

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>

template <class T, class Alloc = std::allocator<T>>
class List {
private:
    // -------- 链接基类：只有两个指针。哨兵用的就是它（不浪费 sizeof(T)）--------
    // 易错点: 若让哨兵也用一个"含 T 存储"的节点类型，每个 List 对象都会白白多出
    //         sizeof(T) 字节。标准库同样把"链接"与"值存储"拆成 base/derived 两层。
    struct Link {
        Link* prev = nullptr;
        Link* next = nullptr;
    };

    // -------- 值节点：继承链接，再挂一块 T 的原始存储 --------
    struct Node : Link {
        // 易错点: 用 unsigned char[] 存 T 而不是直接放 `T value;`。
        //         直接放 T 会强制节点构造时就默认构造 T —— 于是 T 必须可默认构造，
        //         emplace 的"就地构造"也失去意义。用原始存储 + 就位构造才通用。
        alignas(T) unsigned char storage[sizeof(T)];

        // 易错点: 从原始存储取 T* 必须 std::launder —— 编译器可能缓存了这块内存
        //         "没有 T 对象"的信息，launder 告诉它"同一地址现在是活的 T"。
        T*       val() noexcept       { return std::launder(reinterpret_cast<T*>(storage)); }
        const T* val() const noexcept { return std::launder(reinterpret_cast<const T*>(storage)); }
    };

    // -------- 迭代器：对 const 与否做一层模板参数，避免写两份 --------
    template <bool Const>
    class Iterator {
        friend class List;               // 让 List 能调用私有构造 Iterator(Link*)
        template <bool> friend class Iterator;   // 让 iterator 能读 const_iterator 的 node_

    public:
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type        = T;
        using difference_type   = std::ptrdiff_t;
        using pointer           = std::conditional_t<Const, const T*, T*>;
        using reference         = std::conditional_t<Const, const T&, T&>;

        Iterator() noexcept = default;

        // 易错点: iterator → const_iterator 必须是【单向】隐式转换。
        //         这里只在 Const==true 这一侧提供转换构造，反方向不存在 →
        //         想从 const_iterator 转回 iterator 会编译报错（正是我们想要的）。
        template <bool C = Const, std::enable_if_t<C, int> = 0>
        Iterator(const Iterator<false>& o) noexcept : node_(o.node_) {}

        reference operator*()  const noexcept { return *as_node()->val(); }
        pointer   operator->() const noexcept { return as_node()->val(); }

        Iterator& operator++()    noexcept { node_ = node_->next; return *this; }
        Iterator  operator++(int) noexcept { Iterator t = *this; node_ = node_->next; return t; }
        Iterator& operator--()    noexcept { node_ = node_->prev; return *this; }
        Iterator  operator--(int) noexcept { Iterator t = *this; node_ = node_->prev; return t; }

        // 支持 iterator 与 const_iterator 混合比较（双向模板）
        template <bool C2>
        bool operator==(const Iterator<C2>& o) const noexcept { return node_ == o.node_; }
        template <bool C2>
        bool operator!=(const Iterator<C2>& o) const noexcept { return node_ != o.node_; }

    private:
        explicit Iterator(Link* n) noexcept : node_(n) {}
        using node_pointer = std::conditional_t<Const, const Node*, Node*>;
        // 存的是 Link*（非 const）：const 语义只体现在 operator* 的返回类型上。
        node_pointer as_node() const noexcept { return static_cast<node_pointer>(node_); }
        Link* node_ = nullptr;
    };

public:
    using value_type             = T;
    using allocator_type         = Alloc;
    using size_type              = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using reference              = T&;
    using const_reference        = const T&;
    using pointer                = typename std::allocator_traits<Alloc>::pointer;
    using const_pointer          = typename std::allocator_traits<Alloc>::const_pointer;
    using iterator               = Iterator<false>;
    using const_iterator         = Iterator<true>;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // ======================= 构造 / 赋值 / 析构 =======================
    List() : List(Alloc()) {}
    explicit List(const Alloc& a) : alloc_(a), node_alloc_(a) { init_sentinel(); }

    // 拷贝：委托到 List(alloc) 先建好一个合法空表，再逐个 push_back。
    // 之所以委托而不是在函数体里建：委托构造一旦完成，*this 就是个能正常析构的对象，
    // 若 push_back 中途抛异常，析构函数会替我们回收已插入的节点（不漏）。
    // 易错点: 若不用委托、直接在自身上建，构造函数抛异常时析构函数【不会】被调用，
    //         已经连上 sentinel 的节点就泄漏了。
    List(const List& o)
        : List(std::allocator_traits<Alloc>::select_on_container_copy_construction(o.alloc_)) {
        for (const T& v : o) push_back(v);
    }
    List(const List& o, const Alloc& a) : List(a) {
        for (const T& v : o) push_back(v);
    }

    // 移动：O(1) 偷走整条链（含 sentinel 指向），把源复位成空表。
    List(List&& o) noexcept
        : alloc_(std::move(o.alloc_)), node_alloc_(std::move(o.node_alloc_)) {
        init_sentinel();
        if (o.size_ != 0) steal_from(o);
    }
    List(List&& o, const Alloc& a) : List(a) {
        if (alloc_ == o.alloc_) steal_from(o);
        else { for (T& v : o) emplace_back(std::move(v)); o.clear(); }
    }

    List(std::initializer_list<T> il, const Alloc& a = Alloc()) : List(a) {
        for (const T& v : il) push_back(v);
    }

    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    List(It first, It last, const Alloc& a = Alloc()) : List(a) {
        for (; first != last; ++first) emplace_back(*first);
    }

    ~List() { clear(); }

    // 拷贝赋值：copy-and-swap —— 先把副本建好（可能抛），成功后再一次性换入。
    // 若拷贝中途抛异常，*this 仍保持原样（强异常保证）。
    List& operator=(const List& o) {
        if (this != &o) { List tmp(o); swap(tmp); }
        return *this;
    }
    List& operator=(List&& o) {
        if (this == &o) return *this;
        clear();
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_move_assignment::value) {
            alloc_ = o.alloc_;
            node_alloc_ = o.node_alloc_;
        }
        if (alloc_ == o.alloc_) steal_from(o);              // 分配器相同 → O(1) 偷
        else { for (T& v : o) emplace_back(std::move(v)); o.clear(); }  // 否则只能逐元素搬
        return *this;
    }
    List& operator=(std::initializer_list<T> il) { List tmp(il, alloc_); swap(tmp); return *this; }

    allocator_type get_allocator() const noexcept { return alloc_; }

    // ======================= 元素访问 =======================
    reference       front()       noexcept { return *begin(); }
    const_reference front() const noexcept { return *begin(); }
    reference       back()        noexcept { iterator it = end(); --it; return *it; }
    const_reference back()  const noexcept { const_iterator it = end(); --it; return *it; }

    // ======================= 迭代器 =======================
    iterator       begin()  noexcept { return iterator(sentinel_.next); }
    const_iterator begin()  const noexcept { return const_iterator(sentinel_.next); }
    const_iterator cbegin() const noexcept { return begin(); }

    // 易错点: 在 const 成员里取哨兵地址是 const Link*，而迭代器内部存 Link*。
    //         这里 const_cast 是安全的：const 语义已由 operator* 的返回类型守住，
    //         我们绝不会经由这个指针去改元素。标准库的 const_iterator 也这么做。
    iterator       end()    noexcept { return iterator(&sentinel_); }
    const_iterator end()    const noexcept { return const_iterator(const_cast<Link*>(&sentinel_)); }
    const_iterator cend()   const noexcept { return end(); }

    reverse_iterator       rbegin()  noexcept { return reverse_iterator(end()); }
    reverse_iterator       rend()    noexcept { return reverse_iterator(begin()); }
    const_reverse_iterator rbegin()  const noexcept { return const_reverse_iterator(end()); }
    const_reverse_iterator rend()    const noexcept { return const_reverse_iterator(begin()); }

    // ======================= 容量（均为 O(1)）=======================
    bool      empty() const noexcept { return size_ == 0; }
    size_type size()  const noexcept { return size_; }

    // ======================= 修改 =======================
    void clear() noexcept {
        // 训练点: 迭代销毁。
        // 若把节点写成 unique_ptr<Node> 链（node->next 是 unique_ptr），析构 head 时会
        // 递归析构 next、next 的 next……10 万个节点 = 10 万层递归 → 栈溢出（段错误）。
        // 显式循环把"递归深度"降成"循环次数"，O(1) 栈空间。
        Link* p = sentinel_.next;
        while (p != &sentinel_) {
            Link* nxt = p->next;
            destroy_node(static_cast<Node*>(p));
            p = nxt;
        }
        init_sentinel();
        size_ = 0;
    }

    template <class... Args>
    iterator emplace(const_iterator pos, Args&&... args) {
        Node* n = create_node(std::forward<Args>(args)...);
        link_before(pos.node_, n);      // 在 pos 前插入
        ++size_;
        return iterator(n);
    }
    iterator insert(const_iterator pos, const T& v) { return emplace(pos, v); }
    iterator insert(const_iterator pos, T&& v)      { return emplace(pos, std::move(v)); }
    iterator insert(const_iterator pos, size_type count, const T& v) {
        iterator ret(static_cast<Link*>(pos.node_));    // count==0 时返回 pos
        for (size_type i = 0; i < count; ++i) {
            iterator it = emplace(pos, v);
            if (i == 0) ret = it;
        }
        return ret;
    }
    template <class It, class = std::enable_if_t<!std::is_integral_v<It>>>
    iterator insert(const_iterator pos, It first, It last) {
        iterator ret(static_cast<Link*>(pos.node_));
        bool first_ins = true;
        for (; first != last; ++first) {
            iterator it = emplace(pos, *first);
            if (first_ins) { ret = it; first_ins = false; }
        }
        return ret;
    }
    iterator insert(const_iterator pos, std::initializer_list<T> il) {
        return insert(pos, il.begin(), il.end());
    }

    template <class... Args> reference emplace_front(Args&&... args) {
        Node* n = create_node(std::forward<Args>(args)...);
        link_before(sentinel_.next, n);   // 插在第一个元素前
        ++size_;
        return *n->val();
    }
    template <class... Args> reference emplace_back(Args&&... args) {
        Node* n = create_node(std::forward<Args>(args)...);
        link_before(&sentinel_, n);       // 插在哨兵前 = 尾部
        ++size_;
        return *n->val();
    }
    void push_front(const T& v) { emplace_front(v); }
    void push_front(T&& v)      { emplace_front(std::move(v)); }
    void push_back(const T& v)  { emplace_back(v); }
    void push_back(T&& v)       { emplace_back(std::move(v)); }

    void pop_front() noexcept { erase(begin()); }
    void pop_back()  noexcept { erase(iterator(sentinel_.prev)); }

    // erase：只让"被删的那个迭代器"失效，其余迭代器/引用全部保持有效。
    // 返回被删元素的下一个位置——这是删除单个元素时的经典用法。
    iterator erase(const_iterator pos) noexcept {
        Link* n   = pos.node_;
        Link* nxt = n->next;
        unlink(n);
        destroy_node(static_cast<Node*>(n));
        --size_;
        return iterator(nxt);
    }
    iterator erase(const_iterator first, const_iterator last) noexcept {
        Link* end_node = last.node_;
        while (first.node_ != end_node) first = erase(first);
        return iterator(end_node);
    }

    void resize(size_type n) { resize(n, T()); }
    void resize(size_type n, const T& v) {
        while (size_ > n) pop_back();
        while (size_ < n) push_back(v);
    }

    void swap(List& o) noexcept {
        if (this == &o) return;
        // 易错点: 交换后不能用 "sentinel 是否指向自己" 判断空表——此刻它指向【对方】的
        //         sentinel。用 size_ 判断最稳，因为 size_ 也一起被交换了。
        std::swap(sentinel_.next, o.sentinel_.next);
        std::swap(sentinel_.prev, o.sentinel_.prev);
        std::swap(size_, o.size_);
        fix_links_after_swap();
        o.fix_links_after_swap();
        if constexpr (std::allocator_traits<Alloc>::propagate_on_container_swap::value) {
            std::swap(alloc_, o.alloc_);
            std::swap(node_alloc_, o.node_alloc_);
        }
    }

    // ======================= 链表专属操作 =======================
    // 整表 splice：把 other 的【全部节点】搬到 pos 之前，节点不拷贝、迭代器不失效。
    void splice(const_iterator pos, List& other) {
        if (this == &other || other.size_ == 0) return;
        Link* f = other.sentinel_.next;
        Link* l = other.sentinel_.prev;
        Link* p = pos.node_;
        Link* before = p->prev;
        before->next = f; f->prev = before;   // 接前半
        l->next = p;      p->prev = l;        // 接后半
        size_ += other.size_;
        other.init_sentinel();
        other.size_ = 0;
    }
    void splice(const_iterator pos, List&& other) { splice(pos, other); }

    // 单节点 splice：把 it 指向的节点从 other 搬到 pos 之前。
    void splice(const_iterator pos, List& other, const_iterator it) {
        Link* n = it.node_;
        // 原地插入的两种 no-op：pos==it（插到自己前）、或 pos 正好是 it 的后继。
        if (pos.node_ == n || pos.node_ == n->next) return;
        other.unlink(n);
        --other.size_;
        link_before(pos.node_, n);
        ++size_;
    }

    // 区间版：[first, last) 整段搬走。
    void splice(const_iterator pos, List& other, const_iterator first, const_iterator last) {
        if (first.node_ == last.node_) return;
        if (this == &other) {
            if (pos.node_ == last.node_ || pos.node_ == first.node_) return;
            for (Link* p = first.node_; p != last.node_; p = p->next)   // 区间含 pos → no-op
                if (p == pos.node_) return;
        }
        size_type n = 0;
        for (Link* p = first.node_; p != last.node_; p = p->next) ++n;

        Link* f = first.node_;
        Link* l = last.node_;       // 区间是 [f, l)
        Link* fprev = f->prev;      // 拆下前先记好端点前驱
        Link* lprev = l->prev;      // 区间最后一个节点（易错点：必须此刻保存，
                                    //   一旦改了 l->prev 就再也找不回它了）
        // 先把 [f, l) 从 other 摘下
        fprev->next = l;
        l->prev = fprev;
        // 再插到 pos 之前
        Link* p = pos.node_;
        Link* before = p->prev;
        before->next = f;  f->prev = before;
        lprev->next = p;   p->prev = lprev;

        other.size_ -= n;
        size_ += n;
    }

    // reverse：交换每个节点的 prev/next，沿环走一圈即可，零元素移动。
    void reverse() noexcept {
        if (size_ < 2) return;
        Link* cur = &sentinel_;
        do {
            std::swap(cur->next, cur->prev);
            cur = cur->prev;         // 交换后 prev 才是原来的 next
        } while (cur != &sentinel_);
    }

    // sort：稳定归并排序，O(n log n)，【零元素拷贝/移动】——只重连节点指针。
    void sort() { sort_with(std::less<T>()); }
    template <class Cmp> void sort(Cmp cmp) { sort_with(cmp); }

    // merge：两个已排序表归并，稳定（相等时 *this 的元素在前）。
    void merge(List& other) { merge_with(other, std::less<T>()); }
    template <class Cmp> void merge(List& other, Cmp cmp) { merge_with(other, cmp); }

    // unique：删除【连续】重复元素，返回删除个数。
    size_type unique() { return unique_with(std::equal_to<T>()); }
    template <class Pred> size_type unique_with(Pred p) {
        if (size_ < 2) return 0;
        size_type removed = 0;
        iterator it = begin();
        iterator nx = it; ++nx;
        while (nx != end()) {
            if (p(*it, *nx)) { nx = erase(nx); ++removed; }
            else { it = nx; ++nx; }
        }
        return removed;
    }

    // remove_if：删除所有满足 p 的元素，返回删除个数。
    template <class Pred> size_type remove_if(Pred p) {
        size_type removed = 0;
        iterator it = begin();
        while (it != end()) {
            if (p(*it)) { it = erase(it); ++removed; }
            else ++it;
        }
        return removed;
    }
    size_type remove(const T& v) {
        return remove_if([&v](const T& x) { return x == v; });
    }

    friend bool operator==(const List& a, const List& b) {
        if (a.size_ != b.size_) return false;
        const_iterator ia = a.begin(), ib = b.begin();
        for (; ia != a.end(); ++ia, ++ib) if (!(*ia == *ib)) return false;
        return true;
    }

private:
    using node_alloc_type   = typename std::allocator_traits<Alloc>::template rebind_alloc<Node>;
    using node_alloc_traits = std::allocator_traits<node_alloc_type>;

    void init_sentinel() noexcept { sentinel_.next = sentinel_.prev = &sentinel_; }

    // O(1) 偷整条链（假定分配器兼容）
    void steal_from(List& o) noexcept {
        if (o.size_ == 0) return;
        sentinel_.next = o.sentinel_.next;
        sentinel_.prev = o.sentinel_.prev;
        sentinel_.next->prev = &sentinel_;   // 首节点 prev 指回本表哨兵
        sentinel_.prev->next = &sentinel_;   // 尾节点 next 指回本表哨兵
        size_ = o.size_;
        o.init_sentinel();
        o.size_ = 0;
    }
    void fix_links_after_swap() noexcept {
        if (size_ == 0) { init_sentinel(); return; }
        sentinel_.next->prev = &sentinel_;
        sentinel_.prev->next = &sentinel_;
    }

    // 在 pos 之前插入已建好的节点 n
    static void link_before(Link* pos, Link* n) noexcept {
        n->next = pos;
        n->prev = pos->prev;
        pos->prev->next = n;
        pos->prev = n;
    }
    static void unlink(Link* n) noexcept {
        n->prev->next = n->next;
        n->next->prev = n->prev;
    }

    // 分配节点 + 就位构造元素。若元素构造抛异常，必须释放节点（否则泄漏）。
    template <class... Args>
    Node* create_node(Args&&... args) {
        Node* n = node_alloc_traits::allocate(node_alloc_, 1);
        ::new (static_cast<void*>(n)) Node;    // 起 Node 生命周期：prev/next=nullptr，storage 不动
        try {
            std::allocator_traits<Alloc>::construct(alloc_, n->val(), std::forward<Args>(args)...);
        } catch (...) {
            // 易错点: 这一行是"异常安全"的关键。少了它，元素构造失败时节点内存永不回收。
            node_alloc_traits::deallocate(node_alloc_, n, 1);
            throw;
        }
        return n;
    }
    void destroy_node(Node* n) noexcept {
        std::allocator_traits<Alloc>::destroy(alloc_, n->val());
        node_alloc_traits::deallocate(node_alloc_, n, 1);
    }

    static T& value_of(Link* p) noexcept { return *static_cast<Node*>(p)->val(); }

    // -------- 归并排序的递归部分（深度 O(log n)，不会爆栈）--------
    // 排序仅重连指针：比较用 value_of（返回引用），全程不拷贝/移动元素。
    template <class Cmp>
    static Link* merge_linear(Link* a, Link* b, Cmp& cmp) {
        Link dummy;                      // 栈上哑节点，避免对"首元素"特判
        Link* tail = &dummy;
        while (a && b) {
            // 稳定关键：只有 b 严格小于 a 才取 b；相等时取 a（a 来自更靠前的区间）。
            if (cmp(value_of(b), value_of(a))) { tail->next = b; b = b->next; }
            else                               { tail->next = a; a = a->next; }
            tail = tail->next;
        }
        tail->next = a ? a : b;          // 接上剩下的整段
        return dummy.next;
    }
    template <class Cmp>
    static Link* merge_sort(Link* head, size_type n, Cmp& cmp) {
        if (n <= 1) { if (head) head->next = nullptr; return head; }
        size_type half = n / 2;
        Link* mid = head;
        for (size_type i = 0; i < half; ++i) mid = mid->next;
        mid->prev->next = nullptr;       // 切断左半段（用原始 prev 找中点前驱）
        Link* a = merge_sort(head, half, cmp);
        Link* b = merge_sort(mid, n - half, cmp);
        return merge_linear(a, b, cmp);
    }
    // 把线性链（以 nullptr 结尾）重新接成环，并修好所有 prev
    void stitch_from(Link* head) noexcept {
        Link* prev = &sentinel_;
        Link* cur  = head;
        while (cur) { cur->prev = prev; prev = cur; cur = cur->next; }
        prev->next = &sentinel_;
        sentinel_.next = head;
        sentinel_.prev = prev;
    }
    template <class Cmp>
    void sort_with(Cmp cmp) {
        if (size_ < 2) return;
        Link* head = sentinel_.next;
        sentinel_.prev->next = nullptr;  // 断开环 → 线性链
        stitch_from(merge_sort(head, size_, cmp));
    }

    template <class Cmp>
    void merge_with(List& other, Cmp cmp) {
        if (this == &other || other.size_ == 0) return;
        if (size_ == 0) { splice(end(), other); return; }
        iterator it  = begin();
        iterator oit = other.begin();
        while (oit != other.end() && it != end()) {
            if (cmp(*oit, *it)) {        // *oit 应排在 *it 之前
                iterator cur = oit;
                ++oit;
                splice(it, other, cur);  // 单节点搬运，O(1)
            } else {
                ++it;                    // *it <= *oit：this 的元素保持在前面（稳定）
            }
        }
        if (other.size_ != 0) splice(end(), other);
    }

    [[no_unique_address]] Alloc          alloc_{};
    [[no_unique_address]] node_alloc_type node_alloc_{};
    Link      sentinel_;                 // 哨兵：值节点的"环形锚点"
    size_type size_ = 0;
};

// C++20 concepts 核查点：迭代器类别必须名副其实。
static_assert(std::bidirectional_iterator<List<int>::iterator>);
static_assert(std::bidirectional_iterator<List<int>::const_iterator>);

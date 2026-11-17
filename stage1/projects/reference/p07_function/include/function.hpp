#pragma once
// Function<R(Args...)> / MoveOnlyFunction<R(Args...)> —— 类型擦除 + 小缓冲优化（项目 7）
// ============================================================================
// 训练点：类型擦除、SBO（小缓冲优化）、可调用对象的生命周期、std::invoke。
//
// 类型擦除的核心：把"任意可调用对象"的【操作】抽出来做成一张函数指针表（vtable），
// 对象本身只留一个 void* + 一个指向表的指针。于是 Function<int(int)> 能装下 lambda、
// 函数指针、有状态仿函数，而调用方不必知道具体类型。
//
// 两种实现路线（本文件选第 2 种）：
//   1) 虚基类版：内部存一个 `struct Base { virtual R call(...); virtual ~Base(); }`，
//      每个 callable 派生一个具体类。写法直白，但每个对象都带一个 vptr，且基类析构
//      要走动态派发。
//   2) 函数指针表版（本文件）：把 invoke/copy/move/destroy 四个操作放进静态的 VTable，
//      对象操作零虚函数开销、表可共享（同类型共享一张表）。std::function 走的就是这种。
// ============================================================================

#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <typeinfo>
#include <utility>

namespace detail {

template <bool Copyable, class Sig>
class FunctionBase;   // 主模板：只对函数签名 R(Args...) 特化

template <bool Copyable, class R, class... Args>
class FunctionBase<Copyable, R(Args...)> {
public:
    // 选一个够放下"常见小 lambda"的缓冲。太大 → 对象肥胖；太小 → 频繁走堆。
    static constexpr std::size_t kSboSize  = 32;
    static constexpr std::size_t kSboAlign = alignof(std::max_align_t);

    FunctionBase() noexcept = default;
    FunctionBase(std::nullptr_t) noexcept {}

    // 由任意可调用对象构造。排除"同类型"（否则会和拷贝/移动构造打架）。
    template <class F,
              class Df = std::decay_t<F>,
              std::enable_if_t<!std::is_same_v<Df, FunctionBase>, int> = 0,
              std::enable_if_t<std::is_invocable_r_v<R, Df&, Args...>, int> = 0>
    FunctionBase(F&& f) { assign(std::forward<F>(f)); }

    // 拷贝：仅 Copyable 版本存在；MoveOnlyFunction 下此构造被隐式删除。
    FunctionBase(const FunctionBase& o) requires Copyable {
        if (o.vt_ != nullptr) {
            o.vt_->copy(o, *this);
            vt_ = o.vt_;
        }
    }
    FunctionBase& operator=(const FunctionBase& o) requires Copyable {
        if (this != &o) { FunctionBase tmp(o); swap(tmp); }   // copy-and-swap：强异常保证
        return *this;
    }

    // 移动必须 noexcept（容器搬移、swap 都依赖它）。堆对象只搬指针，O(1)。
    FunctionBase(FunctionBase&& o) noexcept {
        if (o.vt_ != nullptr) {
            o.vt_->move(o, *this);
            vt_ = o.vt_;
            o.vt_ = nullptr;                       // 源变空
        }
    }
    FunctionBase& operator=(FunctionBase&& o) noexcept {
        if (this != &o) {
            reset();
            if (o.vt_ != nullptr) {
                o.vt_->move(o, *this);
                vt_ = o.vt_;
                o.vt_ = nullptr;
            }
        }
        return *this;
    }

    ~FunctionBase() { reset(); }

    void reset() noexcept {
        if (vt_ != nullptr) {
            vt_->destroy(*this);
            vt_ = nullptr;
            obj_ = nullptr;
        }
    }

    void swap(FunctionBase& o) noexcept {
        // 易错点: 不能简单地对 obj_/buf_ 做 std::swap——若两个对象都把 callable 放在
        //         各自的内部缓冲里，交换 obj_ 指针会让它们指向【对方的缓冲】，悬空。
        //         用"移进临时再搬回"的方式，各缓冲始终归各自所有。
        FunctionBase t(std::move(*this));
        *this = std::move(o);
        o = std::move(t);
    }
    friend void swap(FunctionBase& a, FunctionBase& b) noexcept { a.swap(b); }

    explicit operator bool() const noexcept { return vt_ != nullptr; }

    R operator()(Args... args) const {
        if (vt_ == nullptr) throw std::bad_function_call();
        // 易错点: 这里是 const 成员却要调用可能非 const 的 callable——std::function 也这样。
        //         它内部 const_cast 掉 const，语义是"Function 对象本身像指针一样不改变其
        //         指向物的可调用性"。存进的是 your callable 的可变副本。
        return vt_->invoke(obj_, std::forward<Args>(args)...);
    }

    // 选做：取回原始可调用对象。类型必须完全匹配（含 lambda 的匿名类型）。
    template <class T> T* target() noexcept {
        return (vt_ != nullptr && vt_->type() == typeid(T)) ? static_cast<T*>(obj_) : nullptr;
    }
    template <class T> const T* target() const noexcept {
        return (vt_ != nullptr && vt_->type() == typeid(T)) ? static_cast<const T*>(obj_) : nullptr;
    }

private:
    template <class F>
    struct Ops {
        // 易错点: 只有"放得下【且】移动不抛"的类型才内联。若移动可能抛，把它放进内部缓冲
        //         会让 Function 的移动也变可能抛——而标准要求 Function 移动 noexcept。
        //         这种类型一律走堆，于是 Function 移动只是搬指针，恒 noexcept。
        static constexpr bool kLocal =
            sizeof(F) <= kSboSize && alignof(F) <= kSboAlign &&
            std::is_nothrow_move_constructible_v<F>;

        static R invoke(void* obj, Args&&... args) {
            if constexpr (std::is_void_v<R>) {
                std::invoke(*static_cast<F*>(obj), std::forward<Args>(args)...);
            } else {
                return std::invoke(*static_cast<F*>(obj), std::forward<Args>(args)...);
            }
        }

        static void copy(const FunctionBase& src, FunctionBase& dst) {
            if constexpr (Copyable) {
                if constexpr (kLocal) {
                    new (dst.buf_) F(*static_cast<const F*>(src.obj_));
                    dst.obj_ = dst.buf_;
                } else {
                    void* mem = ::operator new(sizeof(F));
                    try {
                        new (mem) F(*static_cast<const F*>(src.obj_));
                    } catch (...) {
                        // 易错点: 拷贝构造抛了必须把刚申请的堆内存还回去，否则泄漏。
                        ::operator delete(mem);
                        throw;
                    }
                    dst.obj_ = mem;               // 构造成功后才登记，保证 dst 不指向已释放内存
                }
            } else {
                (void)src; (void)dst;             // MoveOnlyFunction 不会走到这里
            }
        }

        static void move(FunctionBase& src, FunctionBase& dst) noexcept {
            if constexpr (kLocal) {
                F* sf = static_cast<F*>(src.obj_);
                new (dst.buf_) F(std::move(*sf));  // noexcept，因为 kLocal 已排除可抛移动
                dst.obj_ = dst.buf_;
                sf->~F();
                src.obj_ = nullptr;
            } else {
                dst.obj_ = src.obj_;               // 堆：直接搬指针，O(1)
                src.obj_ = nullptr;
            }
        }

        static void destroy(FunctionBase& self) noexcept {
            F* sf = static_cast<F*>(self.obj_);
            if constexpr (kLocal) {
                sf->~F();
            } else {
                sf->~F();
                ::operator delete(self.obj_);
            }
        }

        static const std::type_info& type() noexcept { return typeid(F); }
    };

    struct VTable {
        R    (*invoke)(void*, Args&&...);
        void (*copy)(const FunctionBase&, FunctionBase&);
        void (*move)(FunctionBase&, FunctionBase&) noexcept;
        void (*destroy)(FunctionBase&) noexcept;
        const std::type_info& (*type)() noexcept;
    };

    template <class F>
    static const VTable* vtable() {
        static const VTable vt = {
            &Ops<F>::invoke, &Ops<F>::copy, &Ops<F>::move, &Ops<F>::destroy, &Ops<F>::type,
        };
        return &vt;                                // 同一类型共享一张表
    }

    template <class F>
    void assign(F&& f) {
        using Df = std::decay_t<F>;
        if constexpr (Ops<Df>::kLocal) {
            new (buf_) Df(std::forward<F>(f));
            obj_ = buf_;
        } else {
            void* mem = ::operator new(sizeof(Df));
            try {
                new (mem) Df(std::forward<F>(f));
            } catch (...) {
                ::operator delete(mem);
                throw;
            }
            obj_ = mem;
        }
        vt_ = vtable<Df>();
    }

    void* obj_ = nullptr;                          // 指向 callable：内部缓冲或堆
    alignas(kSboAlign) unsigned char buf_[kSboSize]{};   // SBO 缓冲
    const VTable* vt_ = nullptr;                    // 空状态 = nullptr
};

}  // namespace detail

template <class Sig> using Function         = detail::FunctionBase<true, Sig>;
template <class Sig> using MoveOnlyFunction = detail::FunctionBase<false, Sig>;

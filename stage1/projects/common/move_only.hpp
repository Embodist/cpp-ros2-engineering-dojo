#pragma once
// MoveOnly —— 内部持有 unique_ptr<int>，不可拷贝、可移动的探针
// ============================================================================
// 它解决什么问题：验证容器/算法"只走移动、不走拷贝"。很多接口（push_back、Vector 扩容、
// List::sort）对 move-only 元素必须能工作；如果实现里偷偷拷贝，就会编译失败——这是好事，
// 因为编译期就挡住了。用它做元素类型，能证明你的容器不依赖"元素可拷贝"。
// ============================================================================

#include <memory>

class MoveOnly {
public:
    MoveOnly() : p_(std::make_unique<int>(0)) {}
    explicit MoveOnly(int v) : p_(std::make_unique<int>(v)) {}

    // 为什么: 移动 noexcept —— 容器扩容才会选搬移路径；unique_ptr 的移动本就 noexcept。
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;

    // 易错点: 必须显式 = delete 拷贝。若只声明移动，隐式拷贝会被自动删除（正确），
    //         但显式写出来能在**编译报错信息里明确指出**"拷贝被删了"，定位更快。
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    int value() const noexcept { return *p_; }
    void set(int v) noexcept { *p_ = v; }

private:
    std::unique_ptr<int> p_;
};

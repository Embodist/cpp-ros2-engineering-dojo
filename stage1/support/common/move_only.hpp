#pragma once
// MoveOnly：不可拷贝、可 noexcept 移动的探针，用来证明容器不偷偷依赖元素可拷贝。

#include <memory>

namespace dojo::test {

class MoveOnly {
public:
    MoveOnly() : p_(std::make_unique<int>(0)) {}
    explicit MoveOnly(int v) : p_(std::make_unique<int>(v)) {}

    // 移动 noexcept：容器扩容才会选搬移路径。
    MoveOnly(MoveOnly&&) noexcept = default;
    MoveOnly& operator=(MoveOnly&&) noexcept = default;

    // 易错点：显式 delete 拷贝，编译报错信息会明确指出“拷贝被删了”，比隐式删除更好定位。
    MoveOnly(const MoveOnly&) = delete;
    MoveOnly& operator=(const MoveOnly&) = delete;

    int value() const noexcept { return *p_; }
    void set(int v) noexcept { *p_ = v; }

private:
    std::unique_ptr<int> p_;
};

}  // namespace dojo::test

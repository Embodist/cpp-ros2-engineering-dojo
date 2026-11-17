#pragma once
// LeakCheck：作用域守卫，退出时断言 Tracer 存活数回到进入时的基线。
// 用法：{ LeakCheck lc; ...被测代码... }  // 析构时自动检查
// 依赖 gtest，因此只给 GoogleTest 套件使用。

#include <gtest/gtest.h>

#include "common/tracer.hpp"

namespace dojo::test {

class LeakCheck {
public:
    LeakCheck() : baseline_(Tracer::live) {}

    ~LeakCheck() {
        EXPECT_EQ(Tracer::live, baseline_)
            << "Tracer 泄漏：存活 " << Tracer::live << "（基线 " << baseline_ << "）";
    }

    // 易错点：守卫自己不能可拷贝/可移动，否则析构检查可能跑多次或在错误时机跑。
    LeakCheck(const LeakCheck&) = delete;
    LeakCheck& operator=(const LeakCheck&) = delete;

private:
    long baseline_;
};

}  // namespace dojo::test

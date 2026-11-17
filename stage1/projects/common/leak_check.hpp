#pragma once
// LeakCheck —— 作用域守卫：进入时记录 Tracer 存活基线，退出时断言回到基线（通常为 0）
// ============================================================================
// 它解决什么问题：一个测试里可能忘了某条路径的析构，或者手工 new 后没 delete。
// 用 RAII 守卫——析构函数在作用域退出时自动跑——检查 Tracer 存活数是否回到基线。
//
//   { LeakCheck lc; ...被测代码... }   // 作用域结束时自动检查，失败即 EXPECT 报告
//
// 为什么用基线而不是硬编码 0：有些测试在外层已经持有 Tracer，基线就不是 0。
//   记录进入时的 live 作为基线，退出时相等即无泄漏。
// 注意：它依赖 gtest（EXPECT_EQ），因此只在测试里用。
// ============================================================================

#include <gtest/gtest.h>
#include "common/tracer.hpp"

class LeakCheck {
public:
    LeakCheck() : baseline_(Tracer::live) {}

    ~LeakCheck() {
        EXPECT_EQ(Tracer::live, baseline_)
            << "Tracer 泄漏：存活 " << Tracer::live
            << "（基线 " << baseline_ << "）";
    }

    // 易错点: 守卫对象本身不能可拷贝/可移动。否则它可能被复制，析构检查跑多次
    //         （或在错误的时机跑）。RAII 守卫一律删掉拷贝/移动。
    LeakCheck(const LeakCheck&) = delete;
    LeakCheck& operator=(const LeakCheck&) = delete;

private:
    long baseline_;
};

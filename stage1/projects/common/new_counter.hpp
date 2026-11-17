#pragma once
// NewCounter —— 重载全局 operator new/delete 计数
// ============================================================================
// 它解决什么问题：证明"短串零分配 / 运行期零 malloc / MakeShared 少一次分配"这类命题。
// 只要全局 operator new 被替换成计数版，任何 new（含标准库内部的）都会被记数，
// 于是测试里 reset() 一清零、跑被测代码、读 allocs()，就能得到确定性的分配次数。
//
//   在测量前调用 NewCounter::reset()，测量后读 allocs()/bytes()
//
// 易错点: 计数定义与全局 operator new 替换【必须】放在 .cpp（见 new_counter.cpp），不能塞进
//         头文件。原因有二：(1) 替换函数不能声明为 inline；(2) 放头文件会被多个 TU 各自定义
//         一份，多重定义链接失败。
// ============================================================================

#include <cstddef>

struct NewCounter {
    static void reset() noexcept;              // 计数清零
    static long allocs() noexcept;             // 累计分配次数
    static long bytes()  noexcept;             // 累计分配字节数

    // 供 new_counter.cpp 内部使用
    static void record(std::size_t sz) noexcept;
};

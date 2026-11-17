#pragma once
// NewCounter：重载全局 operator new/delete 计数，用来证明分配次数/字节数。
// 用法：NewCounter::reset(); ...被测代码...; 读 allocs()/bytes()
// 实现必须在 .cpp（见 new_counter.cpp），不能塞进头文件，否则会多重定义。

#include <cstddef>

namespace dojo::test {

struct NewCounter {
    static void reset() noexcept;
    static long allocs() noexcept;
    static long bytes() noexcept;
    static void record(std::size_t size) noexcept;
};

}  // namespace dojo::test

# 04 · MoveOnlyBuffer：零拷贝所有权

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：unique_ptr, span, bounds, move, byte buffer
- **规范对照**：C++ Core Guidelines R.3, R.11, F.24, C.66
- **接口文件**：`stage1/include/dojo/move_buffer.hpp`
- **测试文件**：`stage1/tests/test_move_buffer.cpp`

## 要完成什么
实现独占可增长字节缓冲区；可移动不可复制；read/write 越界抛 out_of_range；resize 保留旧内容，增长部分必须置零；提供 data() const / data()，size() noexcept。需要解释如何用 span 暴露只读视图且不延长生命周期。

## 验收条件
大小/零初始化、resize 保留数据、move 后源 size=0、越界保护、无双重释放。

## 强制工作流
1. 在 `notes/04.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 04`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
4. 让 AI 评审 bug 风险，但由自己动手修；24~72 小时后空白复写核心机制。
5. 自评 [docs/SCORING.md](../../docs/SCORING.md) 的五维度，记录未覆盖风险。

## 只允许的 AI 提示
- “指出我这段实现违反了哪条不变量，先不给修复代码。”
- “给我一个能稳定复现 bug 的最小测试，不要给实现。”
- “围绕异常/内存/并发提供 3 个刁钻边界条件，并解释预期。”

## 思考题（需写笔记）
- 假设对象在第 3 个操作失败，资源归属、已有状态和异常传播分别是什么？
- 哪一种设计最容易出现未定义行为？用什么工具证明？
- 如果把这个模块放进 ROS2 callback，谁负责数据生命周期？

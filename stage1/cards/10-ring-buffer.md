# 10 · 固定容量环形缓冲区

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：array, indices, invariants, overwrite policy
- **规范对照**：C++ Core Guidelines R.11, Per.1, C.20
- **接口文件**：`stage1/include/dojo/ring_buffer.hpp`
- **测试文件**：`stage1/tests/test_ring_buffer.cpp`

## 要完成什么
RingBuffer<T,N>，单线程版本，不使用锁；try_push(T) 满返回 false（不覆盖）；try_pop() 空返回 nullopt；size/capacity；顺序严格 FIFO；避免默认构造 T 的强制要求（可以选 optional<T> 数组）。

## 验收条件
N=1、满/空、反复环绕、move-only T、严格 FIFO；空间固定不增长。

## 强制工作流
1. 在 `notes/10.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 10`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
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

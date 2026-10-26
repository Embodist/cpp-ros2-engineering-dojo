# 08 · 可关闭的有界阻塞队列

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：mutex, condition_variable, spurious wakeup, close
- **规范对照**：C++ Core Guidelines CP.20, CP.22, CP.42, CP.43
- **接口文件**：`stage1/include/dojo/bounded_queue.hpp`
- **测试文件**：`stage1/tests/test_bounded_queue.cpp`

## 要完成什么
BoundedQueue<T>(capacity>0)：push(T) 在满时等待，close 后返回 false；pop() 在空时等待，关闭且排空后返回 nullopt；close() 幂等并唤醒所有等待线程；不丢失已经入队的任务；禁止持锁调用用户回调。

## 验收条件
FIFO；容量严格受限；close 后不能 push；close 后已入队项仍可 pop；生产者消费者并发退出；容量 0 抛 invalid_argument。

## 强制工作流
1. 在 `notes/08.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 08`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
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

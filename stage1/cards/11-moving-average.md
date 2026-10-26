# 11 · 滑动均值：数值稳定与输入约束

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：deque, data invariant, floating point, precision
- **规范对照**：C++ Core Guidelines P.9, F.4, E.6
- **接口文件**：`stage1/include/dojo/moving_average.hpp`
- **测试文件**：`stage1/tests/test_moving_average.cpp`

## 要完成什么
MovingAverage(window>0) 仅保留最近 window 个 double 值；push() 拒绝 NaN/Inf（抛 invalid_argument）；average() 空返回 nullopt；每次 push 摊还 O(1)，避免每次遍历计算。解释累计浮点误差，提出长期漂移的处理办法。

## 验收条件
窗口大小=1；溢出窗口淘汰旧值；空集合；负值；NaN/Inf；小规模随机测试对照朴素实现。

## 强制工作流
1. 在 `notes/11.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 11`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
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

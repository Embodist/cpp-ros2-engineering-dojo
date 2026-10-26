# 05 · Stage 接口与策略组合

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：virtual destructor, polymorphism, unique_ptr, span
- **规范对照**：C++ Core Guidelines C.35, I.11, R.34
- **接口文件**：`stage1/include/dojo/polymorphic_pipeline.hpp`
- **测试文件**：`stage1/tests/test_polymorphic_pipeline.cpp`

## 要完成什么
创建 Stage 抽象接口 process(span<const float>) -> vector<float>；BiasStage(x) 为每项加 x，ClampStage(lo,hi) 夹紧数值；Pipeline 持有多个 unique_ptr<Stage> 顺序调用；禁止加入 nullptr；lo>hi 拒绝。说明组合为何通常优于多重继承。

## 验收条件
空 Pipeline 返回原数据副本；顺序影响结果；动态派发正确；策略释放无泄漏；拒绝空组件。

## 强制工作流
1. 在 `notes/05.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 05`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
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

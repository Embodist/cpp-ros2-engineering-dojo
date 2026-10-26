# 12 · CallbackRegistry：重入与生命周期

- **类别**：核心手写题（代码骨架+可执行测试）
- **关键词**：functional, mutex, callback safety, reentrancy
- **规范对照**：C++ Core Guidelines CP.22, I.11, R.34
- **接口文件**：`stage1/include/dojo/callback_registry.hpp`
- **测试文件**：`stage1/tests/test_callback_registry.cpp`

## 要完成什么
subscribe(callback) 返回递增非零 Token；unsubscribe(token) 返回是否曾存在；publish(value) 必须在不持有注册表锁时调用用户回调；回调可以取消自己或注册新回调。当前 publish 使用快照语义：在开始时已注册的回调本次被调用，即使同轮中被取消；新注册者下轮生效。单次 publish 遇到回调异常原样向调用方传播。

## 验收条件
回调顺序按 Token 升序；自身取消不死锁；新注册回调下一轮收到消息；重复取消 false；并发注册/取消不破坏结构。

## 强制工作流
1. 在 `notes/12.md` 写出状态、不变量、生命周期和最难的异常/竞争路径；先写预期，不要先看答案。
2. 只修改自己的头文件（确有必要时拆分同目录源文件并同步 CMake），**不改测试让它通过**。
3. 执行 `python3 tools/train.py test 12`；自行再补 2 个反例；内存/并发题使用 Sanitizer。
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

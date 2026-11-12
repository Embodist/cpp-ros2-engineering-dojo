# 源码精读：带着具体问题进去，带着可验证证据出来

先手写核心机制，再对照成熟源码。阅读目标从来不是“读完一个仓库”，而是**能解释行为+预测修改影响+用测试证明**。

顺序建议：
1. `01-STL-VECTOR.md`：手写 MiniVector 后，对照工具链 STL 和异常安全。
2. `02-LEVELDB.md`：进入数据库系统，练调用链、状态机、日志、内存索引。
3. `03-ROS2-EXECUTOR.md`：将并发理解迁移到 ROS2 的 executor、callback group、QoS。

每篇至少交付：调用链图、不变量表、失败注入实验、最小复现代码、一个你之前预测错误的点。

扩展任务 [E02·STL 深水区](../stage1/extensions/E02-stl-internals.md) 把 list / rb_tree / traits / EBO 纳入同一套阅读方法，适合在完成 `01-STL-VECTOR.md` 后继续。

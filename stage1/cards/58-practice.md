# 58 · QoS 概念映射

- **知识域**：Engineering, Diagnostics, Robotics
- **类型**：开放实验题（不提供实现和固定测试）
- **规范参考**：C++ Core Guidelines CP.1

## 情境 / 任务
对照 ROS2 reliability/durability/history/depth 设计实验计划。

## 可执行验收
明确哪些由 DDS/ROS2 保证，哪些是应用层约定。独立补充至少两个不在上述要求中的反例，附对应可运行的测试或编译命令；保存实际输出，不得仅写“已验证”。

## 提交文件
- `labs/58/main.cpp` 或分离后的 `include/`、`src/`，本人决定模块结构。
- `labs/58/README.md`：解释为什么这么设计、关键不变量、异常与生命周期。
- `labs/58/evidence.md`：输入、工具命令、实测输出、还未覆盖的条件。

## 使用 AI 的边界
先提交自己的第一版；让 AI 只提出审查问题和测试样例。不要让 AI 一次输出完整实现。

## ROS2 迁移问题
如果此逻辑发生在订阅回调里，数据的复制、线程、安全退出与延迟会发生什么变化？

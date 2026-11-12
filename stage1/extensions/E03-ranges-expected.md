# E03 · C++20 ranges 与 expected：传感器流水线

- **知识域**：C++20, STL, Error Handling
- **类型**：扩展实验题（不提供实现和固定测试）
- **规范参考**：C++ Core Guidelines SL/ES/E 系列；编号以官方页面为准

## 情境 / 任务

实现一条传感器记录处理流水线：

- 用 `std::ranges` 的 filter/transform/take（或等价 view 组合）筛选、转换、截断记录。
- 用 `std::expected`（若工具链不支持则实现最小 `Expected<T,E>`）返回结构化错误，不用魔法值。
- 至少覆盖空输入、时间戳非法、数值越界三类失败。
- 对比 ranges 惰性管道与朴素循环的输出一致性、复制次数和可读性。

## 可执行验收

管道输出与朴素实现完全一致；3 类错误路径有测试；记录编译器/标准版本和对 ranges/expected 的支持情况；用 chrono 测量并注明环境，不用未测量的性能结论。

## 提交文件

- `labs/E03/main.cpp` 或分离后的 `include/`、`src/`。
- `labs/E03/README.md`：view 生命周期、错误建模、与朴素实现的取舍。
- `labs/E03/evidence.md`：编译命令、错误路径输出、测量环境与结果。

## 使用 AI 的边界

先自己写朴素版本和测试，再让 AI 审查 ranges 的悬垂 view、错误通道和边界；不要直接抄完整管道。

## ROS2 迁移问题

如果记录来自订阅回调，ranges/view 的生命周期能否跨回调保存？错误应该走返回值、异常还是日志/状态？

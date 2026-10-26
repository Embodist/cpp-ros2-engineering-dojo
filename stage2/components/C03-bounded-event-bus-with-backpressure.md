# C03 · Bounded Event Bus With Backpressure

- **阶段**：Stage 2，自主设计与独立实现
- **环境**：纯 C++20，无 ROS2 强制依赖
- **前置能力**：至少完成 Stage 1 的对应核心题；详见 stage2/README.md
- **不附完整解决方案**：接口仅作为设计提示，可合理调整，但须解释偏离原因。

## 真实需求
在 Stage1 BoundedQueue 和 CallbackRegistry 上构建有限队列事件总线。

## 起点接口/责任边界
`publish(Event) -> PublishStatus`、`subscribe(Filter, Handler) -> Subscription`、`shutdown()`；队列策略 Block/DropNewest/DropOldest 可配置。

## 必须考虑的边界与故障注入
满队列、零消费者、慢消费者、订阅者退出、关闭竞态；不在锁下执行用户回调。

## 最少交付物
- `stage2/work/C03/include/` 与 `src/`：你自己的实现；创建 target 级 CMake 工程。
- `tests/`：自动测试，含 2 个故障注入场景；并发/内存模块运行相应 Sanitizer。
- `DESIGN.md`：模块边界、数据流/所有权图、关键不变量、接口与错误语义。
- `evidence.md`：实际运行命令、日志、失败前后对照、性能与环境说明。
- `README.md`：从新机器能按步骤复现（ROS2 组件提供 distro/依赖/启动命令）。

## 性能与结果验证
演示不同背压下的消息丢失统计，解释和 ROS2 QoS 的区别。

## 设计评审自问
1. 谁拥有每条消息/回调/线程？其生命周期何时结束？
2. 失败发生在处理中间，会留下什么状态？
3. 高负载时是排队、丢弃还是阻塞？谁负责背压？
4. 性能结论的输入数据、样本数量和硬件条件是什么？
5. 不依赖 AI 能否重写这个组件中的一个核心接口？

## 验收等级
- **Bronze**：基础功能 + README 运行，正常输入有测试。
- **Silver**：考虑所有列举的错误情况、边界 + 稳定自动测试。
- **Gold**：有可复现的故障与修复证据、Sanitizer（适用时）、基准、架构解释。

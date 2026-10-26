# C06 · ROS2 3-Node Sensor Pipeline

- **阶段**：Stage 2，自主设计与独立实现
- **环境**：需安装 ROS2 与 colcon；先做纯 C++ 单元测试
- **前置能力**：至少完成 Stage 1 的对应核心题；详见 stage2/README.md
- **不附完整解决方案**：接口仅作为设计提示，可合理调整，但须解释偏离原因。

## 真实需求
实现 publisher -> processor -> monitor 三节点完整运行链；有 ROS2 可选骨架。

## 起点接口/责任边界
`robot_dojo_pipeline` package 的 sensor_source、sensor_processor、sensor_monitor；用 std_msgs + rclcpp + 参数/Launch。

## 必须考虑的边界与故障注入
10/50/100Hz 发布，改变 QoS depth/reliability 记录结果；节点退出后不能悬挂回调。

## 最少交付物
- `stage2/work/C06/include/` 与 `src/`：你自己的实现；创建 target 级 CMake 工程。
- `tests/`：自动测试，含 2 个故障注入场景；并发/内存模块运行相应 Sanitizer。
- `DESIGN.md`：模块边界、数据流/所有权图、关键不变量、接口与错误语义。
- `evidence.md`：实际运行命令、日志、失败前后对照、性能与环境说明。
- `README.md`：从新机器能按步骤复现（ROS2 组件提供 distro/依赖/启动命令）。

## 性能与结果验证
colcon build、ros2 launch、ros2 topic hz、ros2 node info、测试和简化架构图。

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

# Stage 2：你来设计组件，而不是让我替你写

本阶段 10 个项目文档放在 `components/`，每个都规定需求、接口草案、边界、故障注入、交付物与验收分层；**没有完整实现**。基础题 01～12 的对应关系：

| 组件 | 最相关的 Stage1 | 定位 | 优先级 |
|---|---|---|---|
| C01 Sensor Pipeline | 04,05,07,11 | 纯 C++ 数据处理 | P0 |
| C02 Device Driver | 01,02,07 | 驱动/设备生命周期 | P0 |
| C03 Event Bus | 08,10,12 | 背压/回调 | P0 |
| C04 Scheduler | 08,09,12 | 并发/退出 | P1 |
| C05 Stream Decoder | 04,06,07 | 网络协议安全 | P1（安全特长） |
| C06 ROS2 Nodes | 05,06,11 | ROS2 功能开发 | **P0** |
| C07 Executor Lab | 08,09,12 | ROS2 并发/实时性 | **P0** |
| C08 Sync / TF2 | 07,10,11 | 时间同步、机器人软件 | P1 |
| C09 Agent Security Gate | 06,07,12 | 安全 + ROS2 差异化 | P1 |
| C10 MiniKV | 03,06,07 | 源码深入与系统训练 | 可选 |

建议主线：**C01 → C03 → C06 → C07 → C09**。若 ROS2 已安装，可以从 C06 并行开始。C10 的系统训练很有价值，但求职优先级不应盖过 ROS2 项目。

`ros2_ws/src/robot_dojo_pipeline/` 是一个最小 ROS2 C++ package 框架，含 3 节点及 Launch，核心算法 TODO。无 ROS2 的本地构建不受影响。

生成个人组件目录：
```bash
mkdir -p stage2/work/C01/{include,src,tests}
cp stage2/templates/DESIGN.md stage2/work/C01/DESIGN.md
cp stage2/templates/evidence.md stage2/work/C01/evidence.md
```

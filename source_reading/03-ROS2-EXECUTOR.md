# ROS2 Executor 与 Callback Groups 阅读任务

1. 先运行标准 Publisher/Subscriber，记录 ROS2 distribution 和 rmw implementation。
2. 阅读当前发行版 `rclcpp` 的 executor 相关接口，关注 callback group、wait set、执行实体的归属。
3. 设计两个订阅回调：A 做 100ms 模拟计算，B 做 5ms 工作；分别在 SingleThreadedExecutor / MultiThreadedExecutor、MutuallyExclusive / Reentrant 组运行。
4. 在发布端测消息时间戳，在订阅端记录队列等待/处理耗时；记录 CPU、线程和 QoS 配置。**不能仅凭 executor 线程数声称所有回调一定并发。**
5. 构造互相等待 Future 的场景，分析为什么会发生死锁；修正 callback group/executor 配置或改设计。
6. 尝试切换 RELIABLE/BEST_EFFORT 与 depth，区分中间件机制、网络状态和应用层丢弃。

参考：https://docs.ros.org/en/rolling/How-To-Guides/Using-callback-groups.html 和 https://docs.ros.org/en/rolling/Concepts/Intermediate/About-Executors.html

交付 `notes/source-ros2-executor.md`：回调调度图、启动脚本、统计数据、问题复现与证据。

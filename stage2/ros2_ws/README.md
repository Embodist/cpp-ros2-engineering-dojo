# ROS2 workspace skeleton（需独立安装 ROS2）

在 Linux + 已安装的 ROS2 环境：
```bash
# 将 <distro> 换成你安装且官方支持的发行版名，不要照抄尖括号
source /opt/ros/<distro>/setup.bash
cd stage2/ros2_ws
colcon build --symlink-install --packages-select robot_dojo_pipeline
source install/setup.bash
ros2 launch robot_dojo_pipeline pipeline.launch.py
```

另外开终端：
```bash
source /opt/ros/<distro>/setup.bash
source stage2/ros2_ws/install/setup.bash   # 根据终端当前工作目录调整
ros2 topic list
ros2 topic hz /sensor/raw
ros2 topic echo /sensor/processed
```

`C06` 的要求远高于这个只具备 pass-through 的骨架：要自行实现参数、频率、数据校验、故障输入、延迟统计、可视化诊断和测试。此 ROS2 package **未在当前机器的 ROS2 环境中编译验收**；本仓库的纯 C++ 验收不能替代 ROS2 集成验收。

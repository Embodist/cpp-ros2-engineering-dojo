#!/usr/bin/env bash
# 容器入口：加载 ROS 2 环境，修正挂载仓库的 git 归属检查，然后执行命令。
set -e

ROS_DISTRO="${ROS_DISTRO:-jazzy}"
if [ -f "/opt/ros/${ROS_DISTRO}/setup.bash" ]; then
  # ROS 的 setup 脚本会引用未定义变量，临时关闭 set -u
  set +u
  # shellcheck disable=SC1090
  source "/opt/ros/${ROS_DISTRO}/setup.bash"
  set -u
fi

# 若当前目录正好是 colcon 工作空间，顺手加载其安装环境
if [ -f "$PWD/install/setup.bash" ]; then
  set +u
  # shellcheck disable=SC1091
  source "$PWD/install/setup.bash"
  set -u
fi

# 宿主机挂载进来的仓库可能属于其他 UID，避免 git 报 dubious ownership
git config --global --replace-all safe.directory '*' 2>/dev/null || true

exec "$@"

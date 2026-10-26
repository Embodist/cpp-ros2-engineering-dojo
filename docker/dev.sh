#!/usr/bin/env bash
# ============================================================================
#  ./docker/dev.sh [命令...]
#
#  不传命令时进入容器的 zsh；常用：
#    ./docker/dev.sh                          # zsh
#    ./docker/dev.sh nvim .                   # 直接开 nvim
#    ./docker/dev.sh python3 tools/train.py list
#    ./docker/dev.sh bash -lc 'cd stage2/ros2_ws && colcon build'
#
#  镜像不存在时会自动构建；配置变化后执行 ./docker/build.sh 重建。
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$HERE/.." && pwd)"

command -v docker >/dev/null 2>&1 || {
  echo "未找到 docker CLI" >&2
  exit 1
}

export DOJO_DIR="${DOJO_DIR:-$REPO_ROOT}"
export NVIM_CONFIG_DIR="${NVIM_CONFIG_DIR:-$HERE/vendor/nvim}"
export PI_CONFIG_DIR="${PI_CONFIG_DIR:-$HERE/vendor/pi}"
export DEV_UID="${DEV_UID:-$(id -u)}"
export DEV_GID="${DEV_GID:-$(id -g)}"

for dir in "$NVIM_CONFIG_DIR" "$PI_CONFIG_DIR"; do
  if [ ! -d "$dir" ]; then
    echo "目录不存在：$dir" >&2
    echo "请设置 NVIM_CONFIG_DIR / PI_CONFIG_DIR，或先准备对应的 dotfiles 仓库。" >&2
    exit 1
  fi
done

exec docker compose -f "$HERE/compose.yaml" run --rm dev "$@"

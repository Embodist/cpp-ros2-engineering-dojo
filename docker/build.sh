#!/usr/bin/env bash
# ============================================================================
#  ./docker/build.sh [额外的 docker build 参数...]
#
#  示例：
#    ./docker/build.sh
#    ./docker/build.sh --no-cache
#    BASE_IMAGE=osrf/ros:jazzy-desktop ./docker/build.sh
#    INSTALL_ROBOTICS_LIBS=1 ./docker/build.sh
#
#  需要 BuildKit（Docker 23+ 默认启用），因为使用了命名构建上下文。
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

NVIM_CONFIG_DIR="${NVIM_CONFIG_DIR:-$HERE/vendor/nvim}"
PI_CONFIG_DIR="${PI_CONFIG_DIR:-$HERE/vendor/pi}"
BASE_IMAGE="${BASE_IMAGE:-ros:jazzy-ros-base}"
APT_MIRROR="${APT_MIRROR:-mirrors.tuna.tsinghua.edu.cn}"
DEV_UID="${DEV_UID:-$(id -u)}"
DEV_GID="${DEV_GID:-$(id -g)}"
INSTALL_PLUGINS="${INSTALL_PLUGINS:-1}"
INSTALL_MASON="${INSTALL_MASON:-1}"
INSTALL_ROBOTICS_LIBS="${INSTALL_ROBOTICS_LIBS:-0}"
INSTALL_PREVIEW_DEPS="${INSTALL_PREVIEW_DEPS:-1}"
DOJO_IMAGE="${DOJO_IMAGE:-cpp-ros2-dojo:jazzy}"

for dir in "$NVIM_CONFIG_DIR" "$PI_CONFIG_DIR"; do
  if [ ! -d "$dir" ]; then
    echo "目录不存在：$dir" >&2
    echo "请设置 NVIM_CONFIG_DIR / PI_CONFIG_DIR。" >&2
    exit 1
  fi
done

args=(
  -f "$HERE/Dockerfile"
  --build-context "nvim-config=$NVIM_CONFIG_DIR"
  --build-context "pi-config=$PI_CONFIG_DIR"
  --build-arg "BASE_IMAGE=$BASE_IMAGE"
  --build-arg "APT_MIRROR=$APT_MIRROR"
  --build-arg "DEV_UID=$DEV_UID"
  --build-arg "DEV_GID=$DEV_GID"
  --build-arg "INSTALL_PLUGINS=$INSTALL_PLUGINS"
  --build-arg "INSTALL_MASON=$INSTALL_MASON"
  --build-arg "INSTALL_ROBOTICS_LIBS=$INSTALL_ROBOTICS_LIBS"
  --build-arg "INSTALL_PREVIEW_DEPS=$INSTALL_PREVIEW_DEPS"
  --build-arg "NODE_VERSION=${NODE_VERSION:-22.22.2}"
  --build-arg "NVIM_VERSION=${NVIM_VERSION:-v0.12.5}"
  --build-arg "PI_VERSION=${PI_VERSION:-0.85.1}"
  --build-arg "CLAUDE_CODE_VERSION=${CLAUDE_CODE_VERSION:-2.1.153}"
  -t "$DOJO_IMAGE"
)

for var in HTTP_PROXY HTTPS_PROXY NO_PROXY http_proxy https_proxy no_proxy; do
  if [ -n "${!var:-}" ]; then
    args+=(--build-arg "$var=${!var}")
  fi
done

exec docker build "${args[@]}" "$@" "$HERE"

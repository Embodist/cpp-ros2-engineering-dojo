#!/usr/bin/env bash
# ============================================================================
#  ./docker/build-vscode.sh [额外的 docker build 参数...]
#
#  code-server / VS Code 变体：Debian 13.4 + C++ 工具链 + Neovim + AI + CLI。
#  与主镜像共用 docker/vendor 配置快照与命名构建上下文。
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

NVIM_CONFIG_DIR="${NVIM_CONFIG_DIR:-$HERE/vendor/nvim}"
PI_CONFIG_DIR="${PI_CONFIG_DIR:-$HERE/vendor/pi}"
BASE_IMAGE="${BASE_IMAGE:-debian:13.4}"
APT_MIRROR="${APT_MIRROR:-mirrors.tuna.tsinghua.edu.cn}"
DEV_UID="${DEV_UID:-$(id -u)}"
DEV_GID="${DEV_GID:-$(id -g)}"
INSTALL_PLUGINS="${INSTALL_PLUGINS:-1}"
INSTALL_MASON="${INSTALL_MASON:-1}"
INSTALL_PREVIEW_DEPS="${INSTALL_PREVIEW_DEPS:-1}"
CODE_SERVER_VERSION="${CODE_SERVER_VERSION:-4.143.1}"
DOJO_IMAGE="${DOJO_IMAGE:-cpp-ros2-dojo:vscode}"

for dir in "$NVIM_CONFIG_DIR" "$PI_CONFIG_DIR"; do
  if [ ! -d "$dir" ]; then
    echo "目录不存在：$dir" >&2
    exit 1
  fi
done

args=(
  -f "$HERE/Dockerfile.vscode"
  --build-context "nvim-config=$NVIM_CONFIG_DIR"
  --build-context "pi-config=$PI_CONFIG_DIR"
  --build-arg "BASE_IMAGE=$BASE_IMAGE"
  --build-arg "APT_MIRROR=$APT_MIRROR"
  --build-arg "DEV_UID=$DEV_UID"
  --build-arg "DEV_GID=$DEV_GID"
  --build-arg "INSTALL_PLUGINS=$INSTALL_PLUGINS"
  --build-arg "INSTALL_MASON=$INSTALL_MASON"
  --build-arg "INSTALL_PREVIEW_DEPS=$INSTALL_PREVIEW_DEPS"
  --build-arg "CODE_SERVER_VERSION=$CODE_SERVER_VERSION"
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

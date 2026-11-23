#!/usr/bin/env bash
# ============================================================================
#  ./docker/build-vscode.sh [额外的 docker build 参数...]
#
#  code-server / VS Code 变体：**FROM dojo:latest（主镜像）** + code-server + 扩展。
#  先构建 base：./docker/build.sh
#
#  示例：
#    ./docker/build.sh && ./docker/build-vscode.sh
#    BASE_IMAGE=cpp-ros2-dojo:jazzy ./docker/build-vscode.sh
#    BASE_IMAGE=ghcr.io/embodist/cpp-ros2-engineering-dojo:latest ./docker/build-vscode.sh
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

BASE_IMAGE="${BASE_IMAGE:-cpp-ros2-dojo:jazzy}"
DEV_UID="${DEV_UID:-$(id -u)}"
DEV_GID="${DEV_GID:-$(id -g)}"
CODE_SERVER_VERSION="${CODE_SERVER_VERSION:-4.143.1}"
DOJO_IMAGE="${DOJO_IMAGE:-cpp-ros2-dojo:vscode}"

args=(
  -f "$HERE/Dockerfile.vscode"
  --build-arg "BASE_IMAGE=$BASE_IMAGE"
  --build-arg "DEV_UID=$DEV_UID"
  --build-arg "DEV_GID=$DEV_GID"
  --build-arg "CODE_SERVER_VERSION=$CODE_SERVER_VERSION"
  -t "$DOJO_IMAGE"
)

for var in HTTP_PROXY HTTPS_PROXY NO_PROXY http_proxy https_proxy no_proxy; do
  if [ -n "${!var:-}" ]; then
    args+=(--build-arg "$var=${!var}")
  fi
done

exec docker build "${args[@]}" "$@" "$HERE"

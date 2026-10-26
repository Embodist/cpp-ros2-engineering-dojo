#!/usr/bin/env bash
# ============================================================================
#  刷新 docker/vendor/ 里的 nvim / Pi 配置快照。
#  公开 CI 构建使用 vendor 快照，因此快照必须自包含且不含密钥。
#
#  用法：
#    ./docker/sync-vendor.sh
#    NVIM_DOTFILES=/path/to/nvim PI_DOTFILES=/path/to/dotfiles.pi ./docker/sync-vendor.sh
# ============================================================================
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
NVIM_SRC="${NVIM_DOTFILES:-$HOME/neovim.dotfiles/nvim}"
PI_SRC="${PI_DOTFILES:-$HOME/dotfiles.pi}"

for d in "$NVIM_SRC" "$PI_SRC"; do
  [ -d "$d" ] || { echo "目录不存在：$d" >&2; exit 1; }
done

rm -rf "$HERE/vendor/nvim" "$HERE/vendor/pi"
mkdir -p "$HERE/vendor/nvim" "$HERE/vendor/pi/config" \
         "$HERE/vendor/pi/mcp-servers" "$HERE/vendor/pi/tools"

# nvim 配置：init.lua / lua / snippets / lazy-lock.json 等
cp -a "$NVIM_SRC/." "$HERE/vendor/nvim/"

# Pi 配置：只复制非敏感配置，绝不复制 auth.json
cp -a "$PI_SRC/config/settings.json" "$PI_SRC/config/models.json" "$HERE/vendor/pi/config/"
for d in extensions agents prompts skills; do
  [ -d "$PI_SRC/config/$d" ] && cp -a "$PI_SRC/config/$d" "$HERE/vendor/pi/config/"
done

# 本地零依赖 MCP server
cp -a "$PI_SRC/mcp-servers/." "$HERE/vendor/pi/mcp-servers/"

# 长程自动循环工具（不复制 node_modules）
for f in package.json package-lock.json pi-autorun.js claude-autorun.mjs; do
  [ -f "$PI_SRC/tools/$f" ] && cp -a "$PI_SRC/tools/$f" "$HERE/vendor/pi/tools/"
done

# 文档（镜像内保留；不含密钥）
cp -a "$PI_SRC/README.md" "$HERE/vendor/pi/"
cp -a "$PI_SRC"/PI_*.md "$HERE/vendor/pi/" 2>/dev/null || true

# 安全闸门：vendor 里不允许出现真实 key 或 auth.json
if find "$HERE/vendor" -name auth.json -o -name '*.env' | grep -q .; then
  echo "vendor 中出现 auth.json/.env，拒绝继续" >&2
  exit 1
fi
if grep -RIlE 'sk-[A-Za-z0-9]{20,}' "$HERE/vendor" >/dev/null 2>&1; then
  echo "vendor 中出现疑似真实 API key，拒绝继续：" >&2
  grep -RIlE 'sk-[A-Za-z0-9]{20,}' "$HERE/vendor" >&2
  exit 1
fi

echo "vendor 已更新："
du -sh "$HERE/vendor/nvim" "$HERE/vendor/pi"

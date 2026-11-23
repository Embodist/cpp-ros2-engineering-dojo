#!/usr/bin/env bash
# Claude Code Stop hook：未通过冒烟测试不允许“收工”。
# - 读取 hook stdin，若 stop_hook_active=true 直接放行（防无限循环）。
# - 无 build/ 时跳过（先 python3 tools/train.py build）。
# - 可用 DOJO_STOP_CHECK 覆盖默认命令（工业项目换成自己的测试）。
#   exit 0 = 允许结束；exit 2 = 阻止结束并回传原因给 Claude。
set -uo pipefail

input="$(cat 2>/dev/null || true)"
case "$input" in
  *'"stop_hook_active":true'*|*'"stop_hook_active": true'*) exit 0 ;;
esac

DIR="${CLAUDE_PROJECT_DIR:-$PWD}"
cd "$DIR" 2>/dev/null || exit 0

if [ ! -f build/CMakeCache.txt ]; then
  echo "[stop-verify] 未找到 build/，跳过冒烟测试（先运行: python3 tools/train.py build）" >&2
  exit 0
fi

cmd="${DOJO_STOP_CHECK:-ctest --test-dir build -R '^dojo_smoke$' --output-on-failure}"
log="$(mktemp)"
if eval "$cmd" >"$log" 2>&1; then
  rm -f "$log"
  exit 0
fi

{
  echo "[stop-verify] 冒烟测试未通过，先修复再收工。最近输出："
  tail -n 40 "$log"
} >&2
rm -f "$log"
exit 2

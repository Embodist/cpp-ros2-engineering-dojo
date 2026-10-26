#!/usr/bin/env bash
# 容器内自检：C++ / ROS 2 / Neovim / AI / 现代 CLI
# 用法（在仓库根目录）：
#   ./docker/dev.sh bash docker/smoke.sh
set -uo pipefail

FAIL=0
ok()   { printf '  OK   %s\n' "$*"; }
bad()  { printf '  FAIL %s\n' "$*"; FAIL=1; }
have() { command -v "$1" >/dev/null 2>&1; }
check_cmd() {
  local c="$1"
  if have "$c"; then ok "$c -> $(command -v "$c")"; else bad "missing $c"; fi
}
check_ver() {
  local c="$1"
  if have "$c"; then
    local v
    v="$("$c" --version 2>&1 | head -1 || true)"
    ok "$c ${v:-(--version unavailable)}"
  else
    bad "missing $c"
  fi
}

section() { printf '\n== %s ==\n' "$*"; }

section "C++ toolchain"
for c in gcc g++ clang clang++ clangd clang-tidy clang-format cmake ninja gdb lldb lld valgrind cppcheck ccache heaptrack; do
  check_cmd "$c"
done
cmake --version | head -1
g++ --version | head -1
clangd --version | head -1

section "ROS 2"
for c in ros2 colcon rosdep; do check_cmd "$c"; done
if ros2 --help >/dev/null 2>&1; then ok "ros2 CLI"; else bad "ros2 CLI"; fi
if colcon --help >/dev/null 2>&1; then ok "colcon"; else bad "colcon"; fi

section "Neovim / LazyVim"
check_ver nvim
if nvim --headless "+lua local s=require('lazy').stats(); print('lazy_plugins='..s.count); if s.count < 30 then os.exit(1) end" +qa; then
  ok "lazy plugins load (>=30)"
else
  bad "lazy plugins failed to load"
fi
if [ -d /home/dev/.local/share/nvim/mason/packages ]; then
  ok "mason packages present ($(find /home/dev/.local/share/nvim/mason/packages -mindepth 1 -maxdepth 1 -type d | wc -l) pkgs)"
else
  bad "mason packages missing"
fi
if [ -x /home/dev/.local/share/nvim/mason/bin/codelldb ]; then
  ok "codelldb installed"
else
  bad "codelldb missing"
fi

section "AI / MCP"
check_ver pi
check_ver claude
[ -f /home/dev/.pi/agent/settings.json ] && ok "pi settings" || bad "pi settings"
[ -f /home/dev/.pi/agent/models.json ] && ok "pi models" || bad "pi models"
[ -f /home/dev/.pi/agent/mcp.json ] && ok "pi mcp.json" || bad "pi mcp.json"
[ ! -f /home/dev/.pi/agent/auth.json ] && ok "no baked pi auth.json" || bad "pi auth.json baked into image"
if node /opt/dotfiles.pi/mcp-servers/mini-server.js <<< '{"jsonrpc":"2.0","id":1,"method":"initialize"}' 2>/dev/null | grep -q 'mini-server'; then
  ok "mini MCP server"
else
  bad "mini MCP server"
fi
if node /opt/dotfiles.pi/mcp-servers/searxng-server.js <<< '{"jsonrpc":"2.0","id":1,"method":"initialize"}' 2>/dev/null | grep -q 'searxng-server'; then
  ok "searxng MCP server"
else
  bad "searxng MCP server"
fi

section "Modern CLI / TUI"
for c in rg fd bat fzf jq yq eza zoxide delta dust duf procs btm btop hyperfine sd xh just starship glow ncdu htop sqlite3 tldr shellcheck shfmt yazi ya jnv lazygit gitui; do
  check_cmd "$c"
done
for c in jless json-tui; do
  if have "$c"; then check_cmd "$c"; else printf '  SKIP %s (amd64-only)\n' "$c"; fi
done
for c in ffmpeg ffprobe pdftoppm chafa; do check_cmd "$c"; done
have magick && ok "magick" || { have convert && ok "convert (ImageMagick 6)" || bad "ImageMagick CLI"; }
have 7zz && ok "7zz" || have 7z && ok "7z" || bad "7zip CLI"

section "Dojo build / smoke"
cd /work
if python3 tools/train.py list >/tmp/dojo-list.txt 2>&1; then
  ok "train.py list ($(wc -l </tmp/dojo-list.txt) tasks)"
else
  bad "train.py list"; tail -5 /tmp/dojo-list.txt
fi
if python3 tools/train.py build >/tmp/dojo-build.txt 2>&1; then
  ok "train.py build"
else
  bad "train.py build"; tail -20 /tmp/dojo-build.txt
fi
if ctest --test-dir build -R '^dojo_smoke$' --output-on-failure >/tmp/dojo-ctest.txt 2>&1; then
  ok "ctest dojo_smoke"
else
  bad "ctest dojo_smoke"; tail -20 /tmp/dojo-ctest.txt
fi
if python3 tools/train.py test 01 >/tmp/dojo-test01.txt 2>&1; then
  bad "task 01 unexpectedly PASSed (should be unsolved)"
else
  ok "task 01 fails as expected (unsolved)"
fi

section "ROS 2 Stage 2 build"
if ( cd stage2/ros2_ws \
     && colcon build --symlink-install --packages-select robot_dojo_pipeline >/tmp/dojo-colcon.txt 2>&1 ); then
  ok "colcon build robot_dojo_pipeline"
else
  bad "colcon build robot_dojo_pipeline"; tail -30 /tmp/dojo-colcon.txt
fi

section "Build-time cleanup"
[ ! -e /root/.npm ] && ok "root npm cache absent" || bad "root npm cache present"
[ ! -e /home/dev/.npm ] && ok "dev npm cache absent" || printf '  NOTE dev npm cache present (runtime use)\n'
# ~/.cache 会在运行 nvim/mason 时重新生成，不作为运行时失败条件

printf '\n== RESULT ==\n'
if [ "$FAIL" -eq 0 ]; then
  echo "ALL CHECKS PASSED"
else
  echo "SOME CHECKS FAILED"
  exit 1
fi

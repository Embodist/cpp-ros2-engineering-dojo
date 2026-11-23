#!/usr/bin/env bash
# ============================================================================
#  镜像能力自检（不依赖仓库挂载）。构建后直接运行：
#    docker run --rm <image> dojo-smoke base
#    docker run --rm <image> dojo-smoke vscode
#  或：docker run --rm <image> dojo-smoke          # auto：有 code-server 视为 vscode
#
#  校验的是“镜像内能力”：C++/ROS2、Neovim/Mason、Pi/Claude/ECC、现代 CLI，
#  vscode 版额外校验 code-server、扩展、user/home/workspace。
# ============================================================================
set -uo pipefail

MODE="${1:-auto}"
FAIL=0

ok()   { printf '  OK   %s\n' "$*"; }
bad()  { printf '  FAIL %s\n' "$*"; FAIL=1; }
warn() { printf '  WARN %s\n' "$*"; }
have() { command -v "$1" >/dev/null 2>&1; }
chk()  { if have "$1"; then ok "$1 -> $(command -v "$1")"; else bad "missing $1"; fi; }
sec()  { printf '\n== %s ==\n' "$*"; }

if [ "$MODE" = "auto" ]; then
  if have code-server; then MODE="vscode"; else MODE="base"; fi
fi

sec "identity"
echo "  user=$(id -un) uid=$(id -u) HOME=$HOME"
[ -d "$HOME/.config/nvim" ] && ok "nvim config present" || bad "nvim config missing"

sec "C++ toolchain"
for c in gcc g++ clang clang++ clangd clang-tidy clang-format cmake ninja gdb lldb cppcheck valgrind ccache; do chk "$c"; done

sec "ROS 2 (inherited from base)"
for c in ros2 colcon rosdep; do chk "$c"; done
if bash -c 'set +u; source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash" >/dev/null 2>&1 && ros2 --help >/dev/null 2>&1'; then
  ok "ros2 CLI loads"
else
  bad "ros2 CLI failed to load"
fi

sec "Neovim / Mason"
chk nvim
if [ -d "$HOME/.local/share/nvim/mason/packages" ]; then
  ok "mason packages ($(find "$HOME/.local/share/nvim/mason/packages" -mindepth 1 -maxdepth 1 -type d | wc -l) pkgs)"
else
  bad "mason packages missing"
fi

sec "AI / Claude / core plugins"
chk pi; chk claude
EXPECT_PLUGINS="${DOJO_EXPECT_PLUGINS:-ecc@ecc superpowers@claude-plugins-official clangd-lsp@claude-plugins-official lua-lsp@claude-plugins-official pyright-lsp@claude-plugins-official pr-review-toolkit@claude-plugins-official code-review@claude-plugins-official code-simplifier@claude-plugins-official feature-dev@claude-plugins-official commit-commands@claude-plugins-official claude-code-setup@claude-plugins-official claude-md-management@claude-plugins-official skill-creator@claude-plugins-official explanatory-output-style@claude-plugins-official security-guidance@claude-plugins-official}"
for p in $EXPECT_PLUGINS; do
  if claude plugin list 2>/dev/null | grep -q "$p"; then
    ok "$p installed"
    if claude plugin list 2>/dev/null | grep -A8 "$p" | grep -qi 'enabled'; then
      ok "$p enabled"
    else
      bad "$p not enabled"
    fi
  else
    bad "$p not installed"
  fi
done
[ -d "$HOME/.claude/plugins/cache" ] && ok "claude plugin cache present" || bad "claude plugin cache missing"
if claude plugin details ecc@ecc 2>/dev/null | grep -qi 'Skills'; then
  ok "ecc details readable"
else
  warn "ecc details unavailable (non-fatal)"
fi

sec "Python / uv"
chk python3
python3 --version 2>&1 | head -1
chk uv; uv --version 2>/dev/null | head -1; chk uvx
vdir="$(mktemp -d)"
if uv venv "$vdir/.venv" >/dev/null 2>&1 && [ -x "$vdir/.venv/bin/python" ]; then
  ok "uv venv create"
else
  bad "uv venv create"
fi
rm -rf "$vdir"

sec "Modern CLI / TUI"
for c in rg fd bat fzf jq yq eza zoxide delta dust duf procs btm btop hyperfine sd xh just starship glow ncdu htop sqlite3 tldr yazi ya jnv lazygit gitui; do chk "$c"; done

if [ "$MODE" = "vscode" ]; then
  sec "VS Code / code-server"
  chk code-server
  code-server --version 2>/dev/null | head -1
  [ "$(id -un)" = "user" ] && ok "user=user" || bad "expected user, got $(id -un)"
  [ "$HOME" = "/home/user" ] && ok "HOME=/home/user" || bad "HOME=$HOME"
  [ -d /workspace ] && ok "/workspace exists" || bad "/workspace missing"
  [ -e /work ] && ok "/work -> $(readlink -f /work 2>/dev/null)" || bad "/work missing"
  exts="$(code-server --list-extensions 2>/dev/null || true)"
  # 只强制 Open VSX 可装的扩展；微软专属扩展（cpptools/python 等）不在 Open VSX，best-effort
  for e in llvm-vs-code-extensions.vscode-clangd anthropic.claude-code; do
    if printf '%s\n' "$exts" | grep -qi "^${e}$"; then ok "ext $e"; else bad "ext $e missing"; fi
  done
  sec "inherited base capabilities in vscode"
  for c in clangd cmake nvim ros2; do chk "$c"; done
fi

printf '\n== RESULT ==\n'
if [ "$FAIL" -eq 0 ]; then
  echo "IMAGE SMOKE PASSED ($MODE)"
else
  echo "IMAGE SMOKE FAILED ($MODE)"
  exit 1
fi

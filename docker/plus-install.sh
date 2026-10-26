#!/usr/bin/env bash
# plus 镜像扩容脚本：只在线（GitHub Actions）构建，依赖已有 base 镜像。
# 所有可选项尽量 best-effort，单个网络/仓库失败不阻塞整个镜像。
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive
CURL_OPTS="--retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 20 --max-time 600"
APT_OPTS="-o Acquire::ForceIPv4=true -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30 -o Acquire::Retries=3 -o Acquire::http::Pipeline-Depth=0"

apt-get update $APT_OPTS

# ── 系统 / 网络 / 日志 / C++ 分析 ──────────────────────────────────────────
apt_pkgs=(
  lnav iftop nethogs socat netcat-openbsd tcpdump rclone aria2
  mosh sshfs glances visidata pandoc
  include-what-you-use rr pahole
)
for pkg in "${apt_pkgs[@]}"; do
  if apt-get install -y --no-install-recommends $APT_OPTS "$pkg"; then
    echo "OK apt $pkg"
  else
    echo "WARN apt $pkg (unavailable)"
  fi
done
apt-get clean
rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/*

# ── Python 工具（独立 venv，避免污染系统 Python）───────────────────────────
python3 -m venv /opt/plus-venv
if /opt/plus-venv/bin/pip install --no-cache-dir \
    --index-url https://pypi.tuna.tsinghua.edu.cn/simple \
    pre-commit semgrep; then
  ln -sf /opt/plus-venv/bin/pre-commit /usr/local/bin/pre-commit
  ln -sf /opt/plus-venv/bin/semgrep /usr/local/bin/semgrep
  echo "OK pip pre-commit semgrep"
else
  echo "WARN pip pre-commit/semgrep"
fi

# ── uv / uvx（Astral 官方安装脚本，安装到 /usr/local/bin）────────────────────
if curl $CURL_OPTS -LsSf https://astral.sh/uv/install.sh \
    | env UV_INSTALL_DIR=/usr/local/bin sh; then
  echo "OK uv"
else
  echo "WARN uv"
fi

# ── eget：从 GitHub release 安装预编译 CLI ─────────────────────────────────
if curl $CURL_OPTS -fsSL https://zyedidia.github.io/eget.sh -o /tmp/eget.sh \
  && (cd /tmp && sh eget.sh) \
  && [ -f /tmp/eget ]; then
  install -m 0755 /tmp/eget /usr/local/bin/eget
  echo "OK eget"
else
  echo "WARN eget install"
fi

if command -v eget >/dev/null 2>&1; then
  github_tools=(
    atuinsh/atuin
    jdx/mise
    Wilfred/difftastic
    Canop/broot
    ouch-org/ouch
    sharkdp/hexyl
    zellij-org/zellij
    wagoodman/dive
    jesseduffield/lazydocker
    derailed/k9s
    nektos/act
    gitleaks/gitleaks
    phiresky/ripgrep-all
    bensadeh/tailspin
    imsnif/bandwhich
    YS-L/csvlens
    TomWright/dasel
    tomnomnom/gron
    cantino/mcfly
    stern/stern
  )
  # eget --asset 是子串过滤，可多次；^ 前缀表示反选。
  declare -A eget_assets=(
    [atuinsh/atuin]='atuin-x86_64-unknown-linux-gnu.tar.gz'
    [jdx/mise]='linux-x64.tar.gz'
    [Wilfred/difftastic]='x86_64-unknown-linux-gnu.tar.gz'
    [ouch-org/ouch]='ouch-x86_64-unknown-linux-musl.tar.gz ^sigstore'
    [sharkdp/hexyl]='x86_64-unknown-linux-gnu.tar.gz'
    [zellij-org/zellij]='zellij-x86_64-unknown-linux-musl.tar.gz'
    [wagoodman/dive]='_linux_amd64.tar.gz'
    [derailed/k9s]='k9s_Linux_amd64.tar.gz'
    [phiresky/ripgrep-all]='x86_64-unknown-linux-musl.tar.gz'
    [imsnif/bandwhich]='x86_64-unknown-linux-gnu.tar.gz'
    [YS-L/csvlens]='csvlens-x86_64-unknown-linux-gnu.tar.xz ^sha256'
    [TomWright/dasel]='dasel_linux_amd64 ^gz'
  )
  for repo in "${github_tools[@]}"; do
    filters="${eget_assets[$repo]:-}"
    eget_args=(--to /usr/local/bin)
    for f in $filters; do
      eget_args+=(--asset "$f")
    done
    eget_args+=("$repo")
    if eget "${eget_args[@]}" >/dev/null 2>&1; then
      echo "OK eget $repo"
    else
      echo "WARN eget $repo"
    fi
  done
fi

# ── gdb-dashboard ───────────────────────────────────────────────────────────
if runuser -u dev -- env HOME=/home/dev git -c http.lowSpeedLimit=1000 -c http.lowSpeedTime=60 \
      clone --depth=1 https://github.com/cyrus-and/gdb-dashboard /home/dev/.gdb-dashboard 2>/dev/null; then
  if [ -f /home/dev/.gdb-dashboard/.gdbinit ]; then
    grep -q 'gdb-dashboard' /home/dev/.gdbinit 2>/dev/null \
      || echo 'source /home/dev/.gdb-dashboard/.gdbinit' >> /home/dev/.gdbinit
    chown 1000:1000 /home/dev/.gdbinit /home/dev/.gdb-dashboard 2>/dev/null || true
    chown -R 1000:1000 /home/dev/.gdb-dashboard 2>/dev/null || true
  fi
  echo "OK gdb-dashboard"
else
  echo "WARN gdb-dashboard"
fi

# ── 结果提示（缺失不阻塞镜像）──────────────────────────────────────────────
echo "plus tool check:"
for c in uv uvx atuin mise difft broot ouch hexyl zellij dive lazydocker k9s act \
         gitleaks rga tspin bandwhich csvlens dasel gron mcfly \
         pre-commit semgrep lnav iftop nethogs socat nc tcpdump rclone aria2c \
         mosh sshfs glances vd rr pahole eget; do
  if command -v "$c" >/dev/null 2>&1; then
    echo "  OK   $c"
  else
    echo "  WARN missing $c"
  fi
done

rm -rf /root/.cache /home/dev/.cache /tmp/*

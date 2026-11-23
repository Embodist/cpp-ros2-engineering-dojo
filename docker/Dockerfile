# ============================================================================
#  cpp-ros2-dojo · 统一 C++ / ROS 2 / Neovim / AI 开发容器
#
#  设计目标：
#    · ROS 2 Jazzy + 完整 C++20 工具链（clangd / clang-tidy / sanitizer / lldb）
#    · Neovim 0.12 + 你现有的 LazyVim 配置与插件（构建期预装）
#    · AI：Pi coding agent + Claude Code + 全局 MCP（filesystem/mini/searxng）
#    · tmux / zsh / lazygit / ripgrep / fd / fzf 等基础 CLI
#    · 非 root 用户，UID/GID 可对齐宿主机，挂载工程不产生 root 文件
#
#  构建上下文（BuildKit 命名上下文；CI/公开构建使用仓库内 vendor 快照）：
#    docker build \\
#      --build-context nvim-config=./vendor/nvim \\
#      --build-context pi-config=./vendor/pi \\
#      -t cpp-ros2-dojo:jazzy docker/
#
#  说明见 docker/README.md。
# ============================================================================

ARG BASE_IMAGE=ros:jazzy-ros-base
FROM ${BASE_IMAGE}

ARG DEBIAN_FRONTEND=noninteractive
ARG APT_MIRROR=mirrors.tuna.tsinghua.edu.cn
ARG TARGETARCH

ARG NODE_VERSION=22.22.2
ARG NVIM_VERSION=v0.12.5
ARG LAZYGIT_VERSION=v0.44.1
ARG PI_VERSION=0.85.1
ARG PI_MCP_ADAPTER_VERSION=2.7.2
ARG CLAUDE_CODE_VERSION=2.1.153

# 现代 CLI / TUI（GitHub 预编译）
ARG YAZI_VERSION=v26.9.1
ARG JLESS_VERSION=v0.9.0
ARG JNV_VERSION=v0.7.1
ARG JSON_TUI_VERSION=v1.4.1
ARG BOTTOM_VERSION=0.14.9
ARG DUST_VERSION=v1.2.6
ARG GITUI_VERSION=v0.28.1
ARG XH_VERSION=v0.26.3
ARG STARSHIP_VERSION=v1.27.0
ARG GLOW_VERSION=v3.0.0
ARG PROCS_VERSION=v0.14.12

# 1 = 构建期联网安装 Neovim 插件；0 = 首次启动时再装
ARG INSTALL_PLUGINS=1
# 1 = 预装 Mason 的常用 LSP/格式化工具
ARG INSTALL_MASON=1
# 1 = 额外安装 Eigen/OpenCV/PCL 等机器人库（镜像会显著变大）
ARG INSTALL_ROBOTICS_LIBS=0
# 1 = 安装 yazi 预览依赖（ffmpeg / imagemagick / poppler / chafa / 7zip）
ARG INSTALL_PREVIEW_DEPS=1

# 1 = 构建期预装 ECC 插件（Claude Code，开箱即用；构建时需能访问 GitHub + npm）
# 来源：https://github.com/affaan-m/ECC ；安装后自动 enable，进入容器即可用
ARG INSTALL_ECC=1
ARG ECC_MARKETPLACE=https://github.com/affaan-m/ECC
ARG ECC_PLUGIN=ecc@ecc
# 1 = 安装后裁掉 ECC 的 devDependencies（约省 150MB；若依赖 web3/payment 类 skill 设为 0）
ARG ECC_PRUNE_DEPS=1

# ECC 之外的核心 Claude 工具（官方 marketplace）。默认装 superpowers + clangd-lsp。
# 可覆盖，例如再加 pr-review-toolkit / ralph-loop。
ARG INSTALL_CLAUDE_OFFICIAL=1
ARG CLAUDE_OFFICIAL_MARKETPLACE=anthropics/claude-plugins-official
ARG CLAUDE_EXTRA_PLUGINS="superpowers@claude-plugins-official clangd-lsp@claude-plugins-official lua-lsp@claude-plugins-official pyright-lsp@claude-plugins-official pr-review-toolkit@claude-plugins-official code-review@claude-plugins-official code-simplifier@claude-plugins-official feature-dev@claude-plugins-official commit-commands@claude-plugins-official claude-code-setup@claude-plugins-official claude-md-management@claude-plugins-official skill-creator@claude-plugins-official explanatory-output-style@claude-plugins-official security-guidance@claude-plugins-official"

ARG DEV_UID=1000
ARG DEV_GID=1000

# 所有外网下载统一超时/重试，避免构建无限挂起
ARG CURL_OPTS="--retry 5 --retry-all-errors --retry-delay 2 --connect-timeout 20 --max-time 300"

SHELL ["/bin/bash", "-o", "pipefail", "-c"]

ENV DEBIAN_FRONTEND=noninteractive \
    DISABLE_AUTOUPDATER=1 \
    CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1

# ── apt：国内镜像 + 工具链 ──────────────────────────────────────────────────
RUN set -eux; \
    APT_OPTS="-o Acquire::ForceIPv4=true -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30 -o Acquire::Retries=3 -o Acquire::http::Pipeline-Depth=0"; \
    if [ -n "${APT_MIRROR}" ]; then \
      if [ -f /etc/apt/sources.list.d/ubuntu.sources ]; then \
        sed -i -E "s#https?://(archive|security)\.ubuntu\.com/ubuntu/?#http://${APT_MIRROR}/ubuntu/#g" \
          /etc/apt/sources.list.d/ubuntu.sources; \
      fi; \
      if [ -f /etc/apt/sources.list ]; then \
        sed -i -E "s#https?://(archive|security)\.ubuntu\.com/ubuntu/?#http://${APT_MIRROR}/ubuntu/#g" \
          /etc/apt/sources.list; \
      fi; \
      case "${APT_MIRROR}" in \
        mirrors.tuna.tsinghua.edu.cn|mirrors.ustc.edu.cn) \
          ros_mirror="https://${APT_MIRROR}/ros2/ubuntu" ;; \
        *) ros_mirror="" ;; \
      esac; \
      if [ -n "${ros_mirror}" ] && [ -f /etc/apt/sources.list.d/ros2.sources ]; then \
        sed -i "s#http://packages.ros.org/ros2/ubuntu#${ros_mirror}#g" \
          /etc/apt/sources.list.d/ros2.sources; \
        sed -i 's/^Types:.*deb-src.*/Types: deb/' \
          /etc/apt/sources.list.d/ros2.sources; \
      fi; \
    fi; \
    apt-get update $APT_OPTS; \
    apt-get install -y --no-install-recommends $APT_OPTS \
      build-essential gcc g++ make pkg-config cmake ninja-build ccache \
      clang clangd clang-tidy clang-format clang-tools lld lldb gdb \
      cppcheck valgrind heaptrack \
      libasan8 libtsan2 libubsan1 \
      git curl wget ca-certificates openssh-client gnupg lsb-release \
      ripgrep fd-find fzf bat jq git-delta hyperfine \
      libxml2-utils yamllint cmake-format \
      duf btop eza zoxide sd just shellcheck shfmt \
      ncdu htop sqlite3 tealdeer \
      unzip xz-utils zstd tar less man-db file tree \
      python3 python3-venv python3-pip python3-pytest pipx \
      tmux sudo zsh bash-completion locales \
      gh \
      ros-dev-tools "ros-${ROS_DISTRO}-ros2launch"; \
    if [ "${INSTALL_PREVIEW_DEPS}" = "1" ]; then \
      apt-get install -y --no-install-recommends $APT_OPTS \
        ffmpeg imagemagick poppler-utils chafa 7zip; \
    fi; \
    if [ "${INSTALL_ROBOTICS_LIBS}" = "1" ]; then \
      apt-get install -y --no-install-recommends $APT_OPTS \
        libeigen3-dev libceres-dev libg2o-dev \
        libopencv-dev libpcl-dev liburdfdom-dev libtinyxml2-dev \
        libyaml-cpp-dev libspdlog-dev libfmt-dev libboost-all-dev; \
    fi; \
    ln -sf "$(command -v fdfind)" /usr/local/bin/fd; \
    if command -v batcat >/dev/null 2>&1; then \
      ln -sf "$(command -v batcat)" /usr/local/bin/bat; \
    fi; \
    if ! command -v magick >/dev/null 2>&1 && command -v convert >/dev/null 2>&1; then \
      ln -sf "$(command -v convert)" /usr/local/bin/magick; \
    fi; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/* /tmp/*

# ── Node.js（Pi / Claude Code / MCP 依赖）───────────────────────────────────
RUN set -eux; \
    case "${TARGETARCH:-amd64}" in \
      amd64) narch=x64 ;; \
      arm64) narch=arm64 ;; \
      *) echo "unsupported TARGETARCH: ${TARGETARCH}" >&2; exit 1 ;; \
    esac; \
    curl $CURL_OPTS -fsSL "https://nodejs.org/dist/v${NODE_VERSION}/node-v${NODE_VERSION}-linux-${narch}.tar.xz" \
      -o /tmp/node.tar.xz; \
    tar -xJf /tmp/node.tar.xz -C /usr/local --strip-components=1; \
    rm -f /tmp/node.tar.xz; \
    printf '%s\n' \
      'registry=https://registry.npmmirror.com' \
      'audit=false' \
      'fund=false' \
      'fetch-retries=3' \
      'fetch-timeout=60000' > /etc/npmrc; \
    node --version; npm --version; \
    npm install -g --ignore-scripts --no-audit --no-fund "@earendil-works/pi-coding-agent@${PI_VERSION}"; \
    npm install -g --no-audit --no-fund "@anthropic-ai/claude-code@${CLAUDE_CODE_VERSION}"; \
    npm install -g --no-audit --no-fund "@modelcontextprotocol/server-filesystem@latest"; \
    npm cache clean --force; \
    rm -rf /root/.npm /tmp/*

# ── Neovim / lazygit / yq ───────────────────────────────────────────────────
RUN set -eux; \
    case "${TARGETARCH:-amd64}" in \
      amd64) narch=x86_64; larch=x86_64; yarch=amd64 ;; \
      arm64) narch=arm64;  larch=arm64;  yarch=arm64 ;; \
      *) echo "unsupported TARGETARCH: ${TARGETARCH}" >&2; exit 1 ;; \
    esac; \
    curl $CURL_OPTS -fsSL "https://github.com/neovim/neovim/releases/download/${NVIM_VERSION}/nvim-linux-${narch}.tar.gz" \
      | tar -xz -C /opt; \
    ln -sf "/opt/nvim-linux-${narch}/bin/nvim" /usr/local/bin/nvim; \
    curl $CURL_OPTS -fsSL "https://github.com/jesseduffield/lazygit/releases/download/${LAZYGIT_VERSION}/lazygit_${LAZYGIT_VERSION#v}_Linux_${larch}.tar.gz" \
      | tar -xz -C /tmp lazygit; \
    install -m 0755 /tmp/lazygit /usr/local/bin/lazygit; \
    rm -f /tmp/lazygit; \
    curl $CURL_OPTS -fsSL "https://github.com/mikefarah/yq/releases/latest/download/yq_linux_${yarch}" \
      -o /usr/local/bin/yq; \
    chmod +x /usr/local/bin/yq; \
    nvim --version | head -1; \
    lazygit --version; \
    yq --version; \
    rm -rf /tmp/*

# ── 现代 CLI / TUI（GitHub 预编译；Ubuntu 24.04 没有包的）─────────────────
RUN set -eux; \
    case "${TARGETARCH:-amd64}" in \
      amd64) rustarch=x86_64; debarch=amd64 ;; \
      arm64) rustarch=aarch64; debarch=arm64 ;; \
      *) echo "unsupported TARGETARCH: ${TARGETARCH}" >&2; exit 1 ;; \
    esac; \
    tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT; \
    curl $CURL_OPTS -fsSL "https://github.com/sxyazi/yazi/releases/download/${YAZI_VERSION}/yazi-${rustarch}-unknown-linux-musl.deb" \
      -o "$tmp/yazi.deb"; \
    dpkg -i "$tmp/yazi.deb"; \
    curl $CURL_OPTS -fsSL "https://github.com/ClementTsang/bottom/releases/download/${BOTTOM_VERSION}/bottom-musl_${BOTTOM_VERSION}-1_${debarch}.deb" \
      -o "$tmp/bottom.deb"; \
    dpkg -i "$tmp/bottom.deb"; \
    curl $CURL_OPTS -fsSL "https://github.com/charmbracelet/glow/releases/download/${GLOW_VERSION}/glow_${GLOW_VERSION#v}_${debarch}.deb" \
      -o "$tmp/glow.deb"; \
    dpkg -i "$tmp/glow.deb"; \
    curl $CURL_OPTS -fsSL "https://github.com/bootandy/dust/releases/download/${DUST_VERSION}/dust-${DUST_VERSION}-${rustarch}-unknown-linux-musl.tar.gz" \
      -o "$tmp/dust.tar.gz"; \
    tar -xzf "$tmp/dust.tar.gz" -C "$tmp"; \
    install -m 0755 "$tmp/dust-${DUST_VERSION}-${rustarch}-unknown-linux-musl/dust" /usr/local/bin/dust; \
    curl $CURL_OPTS -fsSL "https://github.com/ducaale/xh/releases/download/${XH_VERSION}/xh-${XH_VERSION}-${rustarch}-unknown-linux-musl.tar.gz" \
      -o "$tmp/xh.tar.gz"; \
    tar -xzf "$tmp/xh.tar.gz" -C "$tmp"; \
    install -m 0755 "$tmp/xh-${XH_VERSION}-${rustarch}-unknown-linux-musl/xh" /usr/local/bin/xh; \
    curl $CURL_OPTS -fsSL "https://github.com/starship/starship/releases/download/${STARSHIP_VERSION}/starship-${rustarch}-unknown-linux-musl.tar.gz" \
      -o "$tmp/starship.tar.gz"; \
    tar -xzf "$tmp/starship.tar.gz" -C "$tmp"; \
    install -m 0755 "$tmp/starship" /usr/local/bin/starship; \
    curl $CURL_OPTS -fsSL "https://github.com/dalance/procs/releases/download/${PROCS_VERSION}/procs-${PROCS_VERSION}-${rustarch}-linux.zip" \
      -o "$tmp/procs.zip"; \
    unzip -q -o "$tmp/procs.zip" -d "$tmp/procs"; \
    install -m 0755 "$tmp/procs/procs" /usr/local/bin/procs; \
    curl $CURL_OPTS -fsSL "https://github.com/extrawurst/gitui/releases/download/${GITUI_VERSION}/gitui-linux-${rustarch}.tar.gz" \
      -o "$tmp/gitui.tar.gz"; \
    tar -xzf "$tmp/gitui.tar.gz" -C "$tmp"; \
    install -m 0755 "$tmp/gitui" /usr/local/bin/gitui; \
    curl $CURL_OPTS -fsSL "https://github.com/ynqa/jnv/releases/download/${JNV_VERSION}/jnv-${rustarch}-unknown-linux-gnu.tar.xz" \
      -o "$tmp/jnv.tar.xz"; \
    tar -xJf "$tmp/jnv.tar.xz" -C "$tmp"; \
    install -m 0755 "$tmp/jnv-${rustarch}-unknown-linux-gnu/jnv" /usr/local/bin/jnv; \
    if [ "${debarch}" = "amd64" ]; then \
      curl $CURL_OPTS -fsSL "https://github.com/PaulJuliusMartinez/jless/releases/download/${JLESS_VERSION}/jless-${JLESS_VERSION}-x86_64-unknown-linux-gnu.zip" \
        -o "$tmp/jless.zip"; \
      unzip -q -o "$tmp/jless.zip" -d "$tmp/jless"; \
      install -m 0755 "$tmp/jless/jless" /usr/local/bin/jless; \
      curl $CURL_OPTS -fsSL "https://github.com/ArthurSonzogni/json-tui/releases/download/${JSON_TUI_VERSION}/json-tui-${JSON_TUI_VERSION#v}-Linux.deb" \
        -o "$tmp/json-tui.deb"; \
      dpkg -i "$tmp/json-tui.deb"; \
    fi; \
    for t in yazi ya btm glow dust xh starship procs gitui jnv; do \
      command -v "$t" >/dev/null; \
    done; \
    if [ "${debarch}" = "amd64" ]; then \
      command -v jless >/dev/null; \
      command -v json-tui >/dev/null; \
    fi; \
    rm -rf /tmp/*

# 日常 CLI 完整性检查（缺一个就让构建失败）
RUN set -eu; \
    for t in rg fd bat fzf jq yq eza zoxide delta dust duf procs btm btop \
             hyperfine sd xh just starship glow ncdu htop sqlite3 tldr \
             shellcheck shfmt yazi ya jnv lazygit; do \
      command -v "$t" >/dev/null || { echo "missing CLI: $t" >&2; exit 1; }; \
    done; \
    if [ "${TARGETARCH:-amd64}" = "amd64" ]; then \
      command -v jless >/dev/null; \
      command -v json-tui >/dev/null; \
    fi

# ── 非 root 用户（UID/GID 对齐宿主机）──────────────────────────────────────
RUN set -eux; \
    if ! getent group "${DEV_GID}" >/dev/null; then \
      groupadd -g "${DEV_GID}" dev; \
    fi; \
    if getent passwd "${DEV_UID}" >/dev/null; then \
      existing="$(getent passwd "${DEV_UID}" | cut -d: -f1)"; \
      if [ "${existing}" != "dev" ]; then \
        usermod -l dev -d /home/dev -m "${existing}"; \
      fi; \
    else \
      useradd -m -u "${DEV_UID}" -g "${DEV_GID}" -s /bin/zsh dev; \
    fi; \
    mkdir -p /home/dev; \
    echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev; \
    chmod 0440 /etc/sudoers.d/dev

# ── 配置：Neovim / Pi / MCP / shell ────────────────────────────────────────
# 注意：只复制配置，不复制 ~/.pi/agent/auth.json；密钥走运行时环境变量。
COPY --from=nvim-config . /home/dev/.config/nvim/
COPY --from=pi-config config/settings.json config/models.json /home/dev/.pi/agent/
COPY --from=pi-config config/extensions/ /home/dev/.pi/agent/extensions/
COPY --from=pi-config config/agents/ /home/dev/.pi/agent/agents/
COPY --from=pi-config config/prompts/ /home/dev/.pi/agent/prompts/
COPY --from=pi-config config/skills/ /home/dev/.pi/agent/skills/
COPY --from=pi-config mcp-servers/ /opt/dotfiles.pi/mcp-servers/
COPY --from=pi-config tools/package.json tools/package-lock.json \
     tools/pi-autorun.js tools/claude-autorun.mjs /opt/dotfiles.pi/tools/
COPY --from=pi-config README.md PI_*.md /opt/dotfiles.pi/
COPY entrypoint.sh /usr/local/bin/entrypoint.sh
COPY smoke-image.sh /usr/local/bin/dojo-smoke
COPY mcp.json /home/dev/.pi/agent/mcp.json
COPY zshrc /home/dev/.zshrc
COPY bashrc /home/dev/.bashrc
COPY tmux.conf /home/dev/.tmux.conf
COPY gitconfig /home/dev/.gitconfig
COPY claude.json /home/dev/.claude.json

RUN set -eux; \
    chmod 0755 /usr/local/bin/entrypoint.sh /usr/local/bin/dojo-smoke; \
    chown -R "${DEV_UID}:${DEV_GID}" /home/dev /opt/dotfiles.pi; \
    mkdir -p /work; \
    chown "${DEV_UID}:${DEV_GID}" /work

# Pi 的 MCP 适配器（写入 ~/.pi/agent/settings.json 的 packages 数组）
RUN timeout 900 runuser -u dev -- env HOME=/home/dev \
      pi install "npm:@cansiny0320/pi-mcp-adapter@${PI_MCP_ADAPTER_VERSION}" \
    && runuser -u dev -- env HOME=/home/dev pi list \
    && rm -rf /home/dev/.npm /home/dev/.cache /root/.npm /tmp/*

# pi-autorun / claude-autorun 依赖
RUN cd /opt/dotfiles.pi/tools \
    && timeout 900 npm install --omit=dev --no-audit --no-fund \
    && npm cache clean --force \
    && chown -R "${DEV_UID}:${DEV_GID}" /opt/dotfiles.pi/tools \
    && rm -rf /root/.npm /home/dev/.npm /tmp/*

# ── 构建期预装 Neovim 插件 + Mason（联网），并在同一层清理缓存 ─────────────
RUN if [ "${INSTALL_PLUGINS}" = "1" ]; then \
      runuser -u dev -- env HOME=/home/dev git config --global http.lowSpeedLimit 1000; \
      runuser -u dev -- env HOME=/home/dev git config --global http.lowSpeedTime 60; \
      timeout 1800 runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 NVIM_ENABLE_AI=1 \
        nvim --headless "+Lazy! restore" +qa \
      || timeout 1800 runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 NVIM_ENABLE_AI=1 \
        nvim --headless "+Lazy! sync" +qa \
      || true; \
      timeout 900 runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 \
        nvim --headless "+TSUpdate" +qa || true; \
      if [ "${INSTALL_MASON}" = "1" ]; then \
        timeout 1800 runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 \
          nvim --headless "+MasonInstall lua-language-server pyright vtsls bash-language-server json-lsp yamlls marksman cmake-language-server dockerfile-language-server stylua codelldb hadolint debugpy ruff prettier markdownlint-cli2 markdown-toc cmakelang cmakelint taplo" +qa \
          || true; \
      fi; \
      rm -rf /home/dev/.cache /home/dev/.npm \
             /home/dev/.local/state/nvim/log \
             /home/dev/.local/share/nvim/mason/.cache \
             /home/dev/.local/share/nvim/mason/staging \
             /home/dev/.local/share/nvim/mason/tmp \
             /root/.npm /tmp/*; \
    fi

# ── 修复 LazyVim 插件安装中的网络遗留（不改用户配置语义）─────────────────
# 1) nvim-ufo 已禁用但 lock 里残留 → 删除 lock 条目
# 2) rainbow-delimiters 的 gitlab 测试子模块不可达 → 去掉子模块并写入 lock
RUN set -eux; \
    LOCK=/home/dev/.config/nvim/lazy-lock.json; \
    LAZY=/home/dev/.local/share/nvim/lazy; \
    git config --global --add safe.directory '*'; \
    find "$LAZY" -maxdepth 1 -name '*.cloning' -exec rm -rf {} + 2>/dev/null || true; \
    if jq -e 'has("nvim-ufo")' "$LOCK" >/dev/null 2>&1; then \
      jq 'del(."nvim-ufo")' "$LOCK" > "$LOCK.tmp" && mv "$LOCK.tmp" "$LOCK"; \
    fi; \
    R="$LAZY/rainbow-delimiters.nvim"; \
    if [ -d "$R/.git" ]; then \
      rm -f "$R/.gitmodules"; \
      rm -rf "$R/test/bin" "$R/.luals/addons/busted" "$R/.luals/addons/luassert"; \
      git -C "$R" config --remove-section submodule..luals/addons/busted || true; \
      git -C "$R" config --remove-section submodule..luals/addons/luassert || true; \
      git -C "$R" config --remove-section submodule.test/bin || true; \
      commit="$(git -C "$R" rev-parse HEAD)"; \
      branch="$(git -C "$R" branch --show-current)"; branch="${branch:-master}"; \
      jq --arg c "$commit" --arg b "$branch" \
        '.["rainbow-delimiters.nvim"] = {"branch": $b, "commit": $c}' \
        "$LOCK" > "$LOCK.tmp" && mv "$LOCK.tmp" "$LOCK"; \
    fi; \
    chown "${DEV_UID}:${DEV_GID}" "$LOCK"; \
    rm -rf /home/dev/.cache /home/dev/.local/state/nvim/log /tmp/*

# 构建期自检：插件真的加载、CLI 真的可用；顺手清理验证产生的缓存
RUN set -eu; \
    if [ "${INSTALL_PLUGINS}" = "1" ]; then \
      runuser -u dev -- env HOME=/home/dev \
        nvim --headless "+lua local s=require('lazy').stats(); print('lazy_plugins='..s.count); if s.count < 30 then os.exit(1) end" +qa; \
    fi; \
    clangd --version | head -1; \
    cmake --version | head -1; \
    pi --version; \
    claude --version; \
    set +u; \
    source "/opt/ros/${ROS_DISTRO:-jazzy}/setup.bash"; \
    ros2 --help >/dev/null; \
    colcon --help >/dev/null 2>&1 || true; \
    set -u; \
    rm -rf /home/dev/.cache /home/dev/.local/state/nvim/log \
           /root/.claude* /root/.pi /root/.npm /root/.cache \
           /root/.ros /root/.colcon /tmp/*

# ── oh-my-zsh / oh-my-tmux / TPM + 日常终端工具 ───────────────────────────
RUN set -eux; \
    APT_OPTS="-o Acquire::ForceIPv4=true -o Acquire::http::Timeout=30 -o Acquire::https::Timeout=30 -o Acquire::Retries=3 -o Acquire::http::Pipeline-Depth=0"; \
    apt-get update $APT_OPTS; \
    apt-get install -y --no-install-recommends $APT_OPTS \
      direnv entr strace tig asciinema rsync pv mtr-tiny \
      lsof net-tools bind9-dnsutils nload xxd git-lfs; \
    apt-get clean; \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/*; \
    clone3() { i=1; while [ "$i" -le 3 ]; do \
        runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 git -c http.lowSpeedLimit=1000 -c http.lowSpeedTime=60 \
          clone --depth=1 "$1" "$2" && return 0; \
        i=$((i+1)); sleep 2; \
      done; return 1; }; \
    [ -d /home/dev/.oh-my-zsh ] || clone3 https://github.com/ohmyzsh/ohmyzsh.git /home/dev/.oh-my-zsh || true; \
    mkdir -p /home/dev/.oh-my-zsh/custom/plugins; \
    [ -d /home/dev/.oh-my-zsh/custom/plugins/zsh-autosuggestions ] || \
      clone3 https://github.com/zsh-users/zsh-autosuggestions /home/dev/.oh-my-zsh/custom/plugins/zsh-autosuggestions || true; \
    [ -d /home/dev/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting ] || \
      clone3 https://github.com/zsh-users/zsh-syntax-highlighting /home/dev/.oh-my-zsh/custom/plugins/zsh-syntax-highlighting || true; \
    if ! grep -q 'oh-my-zsh.sh' /home/dev/.zshrc; then \
      { printf '%s\n' \
          'export ZSH="$HOME/.oh-my-zsh"' \
          'ZSH_THEME="robbyrussell"' \
          'plugins=(git zsh-autosuggestions zsh-syntax-highlighting)' \
          '[ -f "$ZSH/oh-my-zsh.sh" ] && source "$ZSH/oh-my-zsh.sh"'; \
        cat /home/dev/.zshrc; } > /home/dev/.zshrc.tmp; \
      mv /home/dev/.zshrc.tmp /home/dev/.zshrc; \
    fi; \
    grep -q 'direnv hook' /home/dev/.zshrc || \
      echo 'command -v direnv >/dev/null 2>&1 && eval "$(direnv hook zsh)"' >> /home/dev/.zshrc; \
    grep -q 'alias t=' /home/dev/.zshrc || \
      echo "alias t='tmux new -A -s main'" >> /home/dev/.zshrc; \
    if [ ! -d /home/dev/.tmux/.git ]; then \
      rm -rf /home/dev/.tmux; \
      clone3 https://github.com/gpakosz/.tmux.git /home/dev/.tmux || true; \
    fi; \
    if [ -f /home/dev/.tmux/.tmux.conf ]; then \
      ln -sfn /home/dev/.tmux/.tmux.conf /home/dev/.tmux.conf; \
      cp /home/dev/.tmux/.tmux.conf.local /home/dev/.tmux.conf.local; \
    else \
      [ -f /home/dev/.tmux.conf ] || : > /home/dev/.tmux.conf; \
      grep -q 'source-file ~/.tmux.conf.local' /home/dev/.tmux.conf || \
        echo 'source-file ~/.tmux.conf.local' >> /home/dev/.tmux.conf; \
      touch /home/dev/.tmux.conf.local; \
    fi; \
    chown -R "${DEV_UID}:${DEV_GID}" /home/dev/.tmux 2>/dev/null || true; \
    runuser -u dev -- mkdir -p /home/dev/.tmux/plugins; \
    for spec in \
      "tpm tmux-plugins/tpm" \
      "tmux-resurrect tmux-plugins/tmux-resurrect" \
      "tmux-continuum tmux-plugins/tmux-continuum" \
      "tmux-sensible tmux-plugins/tmux-sensible" \
      "tmux-fzf sainnhe/tmux-fzf"; do \
      set -- $spec; pname="$1"; prepo="$2"; \
      [ -d "/home/dev/.tmux/plugins/$pname" ] || \
        clone3 "https://github.com/$prepo" "/home/dev/.tmux/plugins/$pname" || true; \
    done; \
    if ! grep -q "^set -g @plugin 'tmux-plugins/tpm'" /home/dev/.tmux.conf.local 2>/dev/null; then \
      printf '%s\n' \
        "set -g @plugin 'tmux-plugins/tpm'" \
        "set -g @plugin 'tmux-plugins/tmux-sensible'" \
        "set -g @plugin 'tmux-plugins/tmux-resurrect'" \
        "set -g @plugin 'tmux-plugins/tmux-continuum'" \
        "set -g @plugin 'sainnhe/tmux-fzf'" \
        "set -g @continuum-restore 'on'" \
        "set -g @continuum-save-interval '15'" \
        "set -g @resurrect-capture-pane-contents 'on'" \
        "run '~/.tmux/plugins/tpm/tpm'" \
        "set -g default-terminal 'tmux-256color'" \
        "set -as terminal-features ',xterm*:RGB'" \
        "set -g mouse on" \
        "set -g history-limit 200000" \
        "set -g history-file ~/.tmux_history" \
        "set -g allow-passthrough on" \
        "set -g set-clipboard on" \
        "set -g focus-events on" \
        "set -g escape-time 0" \
        "set -g base-index 1" \
        "setw -g pane-base-index 1" \
        "set -g renumber-windows on" \
        "setw -g mode-keys vi" \
        >> /home/dev/.tmux.conf.local; \
    fi; \
    chown -R "${DEV_UID}:${DEV_GID}" /home/dev/.oh-my-zsh /home/dev/.tmux /home/dev/.tmux.conf.local /home/dev/.zshrc; \
    chown -h "${DEV_UID}:${DEV_GID}" /home/dev/.tmux.conf; \
    rm -rf /home/dev/.cache /home/dev/.npm /tmp/*

# ── Mason LSP/工具补装（独立层，避免重跑插件 clone）────────────────────────
COPY mason-install.lua /opt/dojo/mason-install.lua
RUN if [ "${INSTALL_PLUGINS}" = "1" ] && [ "${INSTALL_MASON}" = "1" ]; then \
      timeout 1200 runuser -u dev -- env HOME=/home/dev \
        nvim --headless "+luafile /opt/dojo/mason-install.lua" || true; \
      rm -rf /home/dev/.cache /home/dev/.local/state/nvim/log /tmp/*; \
    fi

# ── Claude 插件（构建期预装并默认启用）──────────────────────────────────────
# ECC（ecc@ecc）+ 官方 marketplace 的核心工具（默认 superpowers、clangd-lsp）。
# 以 dev 用户装到 /home/dev/.claude；claude plugin install 自动 enable。
# npm 走 /etc/npmrc 镜像源；安装后可选 npm prune 裁掉 devDependencies。
RUN if [ "${INSTALL_ECC}" = "1" ]; then \
      set -eux; \
      runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 \
        git config --global http.lowSpeedLimit 1000; \
      runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 \
        git config --global http.lowSpeedTime 60; \
      runuser -u dev -- env HOME=/home/dev \
        claude plugin marketplace add "${ECC_MARKETPLACE}"; \
      runuser -u dev -- env HOME=/home/dev \
        claude plugin install "${ECC_PLUGIN}" --scope user; \
      runuser -u dev -- env HOME=/home/dev \
        claude plugin enable "${ECC_PLUGIN}" || true; \
      if [ "${INSTALL_CLAUDE_OFFICIAL}" = "1" ]; then \
        runuser -u dev -- env HOME=/home/dev GIT_TERMINAL_PROMPT=0 \
          claude plugin marketplace add "${CLAUDE_OFFICIAL_MARKETPLACE}" || true; \
        for p in ${CLAUDE_EXTRA_PLUGINS}; do \
          runuser -u dev -- env HOME=/home/dev \
            claude plugin install "$p" --scope user || true; \
          runuser -u dev -- env HOME=/home/dev \
            claude plugin enable "$p" || true; \
        done; \
      fi; \
      if [ "${ECC_PRUNE_DEPS}" = "1" ]; then \
        find /home/dev/.claude/plugins/cache -maxdepth 6 -type d -name node_modules -print0 \
          | while IFS= read -r -d '' nm; do \
              parent="$(dirname "$nm")"; \
              if [ -f "$parent/package.json" ]; then \
                ( cd "$parent" && runuser -u dev -- env HOME=/home/dev \
                    npm prune --omit=dev --no-audit --no-fund ) || true; \
              fi; \
            done; \
      fi; \
      runuser -u dev -- env HOME=/home/dev claude plugin list; \
      rm -rf /home/dev/.npm /root/.npm /home/dev/.cache /tmp/*; \
    fi

# ── 运行时 ──────────────────────────────────────────────────────────────────
ENV LANG=C.UTF-8 \
    LC_ALL=C.UTF-8 \
    TERM=xterm-256color \
    EDITOR=nvim \
    VISUAL=nvim \
    SHELL=/bin/zsh \
    DISABLE_AUTOUPDATER=1 \
    CLAUDE_CODE_DISABLE_NONESSENTIAL_TRAFFIC=1 \
    HOME=/home/dev

USER dev
WORKDIR /work
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["zsh", "-li"]

# 统一开发容器：C++ / ROS 2 / Neovim / AI

一套容器解决训练营的全部环境问题：

- **C++20 工具链**：GCC 13、Clang/LLVM、clangd、clang-tidy、clang-format、CMake、Ninja、gdb、lldb、ccache、cppcheck、valgrind、heaptrack、ASan/TSan/UBSan。
- **ROS 2 Jazzy**：`rclcpp`、`colcon`、`rosdep`、`ros2` CLI；需要 rviz2 / Gazebo 时切到 desktop 基础镜像。
- **Neovim 0.12 + LazyVim**：直接使用 `~/neovim.dotfiles/nvim` 的配置，构建期按 `lazy-lock.json` 预装插件、treesitter 解析器和常用 Mason 工具。
- **AI**：Pi coding agent、Claude Code CLI、Pi MCP（filesystem / mini / searxng）、nvim 内的 CodeCompanion/Copilot/sidekick（默认关闭）。
- **现代 CLI / TUI**：`rg`、`fd`、`bat`、`eza`、`fzf`、`zoxide`、`starship`、`jq`、`yq`、`jnv`、`jless`、`json-tui`、`yazi`、`glow`、`btop`/`btm`、`procs`、`dust`/`duf`/`ncdu`、`lazygit`/`gitui`、`delta`、`xh`、`hyperfine`、`sd`、`just`、`tldr`。
- **非 root + UID/GID 对齐宿主机**，挂载工程不会产生 root 文件。

---

## 1. 前置条件

- Docker 23+（默认启用 BuildKit，命名构建上下文需要它）
- Docker daemon 正在运行
- 仓库已包含非敏感配置快照 `docker/vendor/`（供 CI/公开构建使用），**不需要**本机 dotfiles
- 可选：本机 SearXNG 监听 `127.0.0.1:11199`，或使用 compose 里的 `search` profile

想用本机最新 nvim/Pi 配置时：

```bash
./docker/sync-vendor.sh                 # 从 ~/neovim.dotfiles 与 ~/dotfiles.pi 刷新 vendor
# 或者只在本次构建临时覆盖：
NVIM_CONFIG_DIR=/path/to/neovim.dotfiles/nvim \
PI_CONFIG_DIR=/path/to/dotfiles.pi \
./docker/build.sh
```

---

## 2. 快速开始

在训练营仓库根目录：

```bash
./docker/dev.sh                          # 进入容器 zsh（首次会自动构建）
./docker/dev.sh nvim .                   # 直接打开 nvim
./docker/dev.sh python3 tools/train.py list
./docker/dev.sh bash -lc 'cd stage2/ros2_ws && colcon build --symlink-install'
```

显式重建镜像（改了 nvim/Pi 配置后）：

```bash
./docker/build.sh
BASE_IMAGE=osrf/ros:jazzy-desktop ./docker/build.sh   # 需要 rviz2/Gazebo
```

首次构建会下载 ROS 基础镜像、apt 包、Node、Neovim 插件和 Mason 工具，通常需要 15–40 分钟。网络受限时可传代理：

```bash
HTTPS_PROXY=http://127.0.0.1:46000 ./docker/build.sh
```

国内网络如果卡在 `load metadata for docker.io/library/ros:jazzy-ros-base`，先用镜像源拉取并覆盖基础镜像：

```bash
docker pull docker.m.daocloud.io/library/ros:jazzy-ros-base
BASE_IMAGE=docker.m.daocloud.io/library/ros:jazzy-ros-base ./docker/build.sh
```

也支持把常用变量写进 `docker/.env`：

```bash
cp docker/.env.example docker/.env
# 编辑 DOJO_DIR / NVIM_CONFIG_DIR / PI_CONFIG_DIR / 密钥
docker compose -f docker/compose.yaml run --rm dev
```

> `docker/.env` 已被 `.gitignore` 忽略，不要把真实密钥提交进仓库。

### 公开镜像 / CI

| Registry | base | VS Code 变体 | plus（CI-only） |
| :--- | :--- | :--- | :--- |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest` | `...:vscode` | `...:plus` |
| 阿里云 ACR rapid7 | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest` | `...:vscode` | `...:plus`（需开启 ACR 自动推送或手动同步） |

```bash
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus
```

- CI：`.github/workflows/docker-image.yml`，`docker/**`（Dockerfile、vendor 配置、构建脚本）变更时触发；PR 只构建不推送，`main`/`master` 推送 GHCR（主镜像 `latest`，VS Code 变体 `vscode`）。
- GHCR package 已设为 **Public**，匿名 `docker manifest inspect` 已对 `latest` / `vscode` / `plus` 验证通过；外部无需 `docker login`。
- CI 可选推 ACR：在仓库 Variables 设置 `ACR_ENABLED=true`，Secrets 设置 `ACR_USERNAME` / `ACR_PASSWORD`，即把同一 tag 同步到 `rapid7` 空间。
- 最新构建记录见 [IMAGE.md](IMAGE.md)。

### VS Code / code-server 变体

```bash
./docker/build-vscode.sh
docker compose -f docker/compose.yaml --profile vscode up -d vscode
# 浏览器打开 http://localhost:8080，默认密码 user
# 改密码：VSCODE_PASSWORD=... docker compose ... up -d vscode
```

- **基于 `dojo:latest`（主镜像）**，叠加 code-server + 扩展；继承 C++/ROS2/Neovim/AI/ECC 等完整能力（先构建 base）。
- 包含 code-server、C++ 工具链、Neovim（默认预装插件/Mason）、Pi/Claude Code、现代 CLI/TUI、zsh/oh-my-zsh/tmux/helix。
- code-server 扩展走 Open VSX；示例里的 Microsoft 专属扩展可能安装失败，构建会继续并打印 `WARN extension ...`。
- 想更快/更小：`INSTALL_PLUGINS=0 ./docker/build-vscode.sh`（不预装 Neovim 插件）。

### Plus 版本（CI-only，依赖 base）

**plus 不在本地构建**，只由 GitHub Actions 构建；它直接 `FROM` 同一 commit 的 base 镜像，只追加工具，不重复安装 base 组件。

- 触发：`docker/**` 变更或 `workflow_dispatch`；CI 先构建 base/vscode，再运行 `build-plus` job。
- 依赖：`ghcr.io/...:sha-<commit>`（CI 自动传入 `BASE_IMAGE`；默认 `:latest`）。
- 标签：`:plus`、`:plus-sha-<commit>`。
- ACR：设置 `ACR_ENABLED=true` + `ACR_USERNAME`/`ACR_PASSWORD` 后，CI 会把 `:plus` 同步到 rapid7。

包含（尽量 best-effort，缺失会打印 `WARN`）：

- 系统/网络/日志/数据：`lnav`、`iftop`、`nethogs`、`socat`、`netcat`、`tcpdump`、`rclone`、`aria2`、`mosh`、`sshfs`、`glances`、`visidata`、`pandoc`
- C++/调试：`include-what-you-use`、`rr`、`pahole`、`gdb-dashboard`
- Python：`uv`、`uvx`
- 质量/安全：`pre-commit`、`semgrep`、`gitleaks`
- eget 预编译 CLI：`atuin`、`mise`、`difftastic`、`broot`、`ouch`、`hexyl`、`zellij`、`dive`、`lazydocker`、`k9s`、`act`、`tailspin`、`bandwhich`、`csvlens`、`dasel`、`gron`、`mcfly`、`stern`
- 当前例外：`rga` 资产匹配失败、`iwyu` Ubuntu 源没有、`bloaty` 上游无预编译；其余工具已装。

注意：

- `tcpdump` / `iftop` / `nethogs` / `bandwhich` 需要 `NET_ADMIN` 或 host 网络。
- `dive` / `lazydocker` 需要挂载 `/var/run/docker.sock`；`k9s` 需要 kubeconfig。
- `rr` / `perf` 等需要额外内核权限。
- eget 工具和 pip 工具是 best-effort；某次 GitHub 网络失败只会在构建日志里出现 `WARN`，不会阻塞镜像。
- plus 只在 CI 构建；当前 GHCR digest `sha256:a44fd522…`（commit `5be8430`，含 `uv/uvx`）。ACR `:plus` 需要开启 `ACR_ENABLED` 或手动同步。

---

## 3. 日常使用

### 训练营命令

```bash
python3 tools/train.py list
python3 tools/train.py build
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py test 08 --sanitize tsan
```

### ROS 2 / Stage 2

```bash
source /opt/ros/jazzy/setup.bash
cd stage2/ros2_ws
colcon build --symlink-install --packages-select robot_dojo_pipeline
source install/setup.bash
ros2 launch robot_dojo_pipeline pipeline.launch.py
```

另一个终端：

```bash
source /opt/ros/jazzy/setup.bash
source stage2/ros2_ws/install/setup.bash
ros2 topic list
ros2 topic hz /sensor/raw
```

### Neovim / clangd

工程以**宿主机相同绝对路径**挂载，容器内生成的 `compile_commands.json` 不会因为路径不同而让 clangd 失效。先构建一次：

```bash
python3 tools/train.py build      # 生成 build/compile_commands.json
nvim .
```

若改动过 CMake 配置，重新执行上面的 build；必要时删掉旧的 `build/` 再生成。

### 现代 CLI / TUI 速查

zsh/bash 里已经接好 `eza`、`bat`、`starship`、`zoxide`：

```bash
rg TODO stage1                  # ripgrep
fd '\.hpp$' stage1              # fd
bat stage1/cards/01-scope-exit.md
eza -alF --git                  # ls/ll 已指向 eza
yazi                            # 文件管理器，别名 y
z stage1                        # zoxide 跳转（已自动初始化）
```

JSON / YAML：

```bash
jq . stage1/manifest.json
jnv stage1/manifest.json        # 交互式 jq TUI
jless stage1/manifest.json      # JSON 浏览器（amd64）
json-tui stage1/manifest.json   # JSON TUI（amd64）
yq '.providers' ~/.pi/agent/models.json
```

系统 / 磁盘 / 进程：

```bash
btop          # 系统监控
btm           # bottom
procs         # 进程查看
dust .        # 目录占用
duf           # 磁盘使用
ncdu          # 交互式磁盘分析
```

Git / 其他：

```bash
lazygit       # Git TUI
gitui         # 另一种 Git TUI
git diff      # delta 已配置为分页器
glow README.md
xh GET https://api.github.com
hyperfine 'cmake --build build'
sd 'old' 'new' file
just --list
tldr tar
```

- `yazi` 的图片/视频/PDF/压缩包预览依赖 `ffmpeg`、`imagemagick`、`poppler-utils`、`chafa`、`7zip`，默认安装；`INSTALL_PREVIEW_DEPS=0 ./docker/build.sh` 可精简镜像。
- 容器内自检：`./docker/dev.sh bash docker/smoke.sh`。
- `jless` / `json-tui` 只有 amd64 预编译；arm64 用 `jnv`。
- `starship` 需要终端支持 Nerd Font；没有字体时不影响功能，只是图标可能显示为方块。
- 更完整的“已内置 / 建议下一批 / 容器内慎用”分析见 [docs/TERMINAL_TOOLS.md](../docs/TERMINAL_TOOLS.md)。

### Shell / tmux

- **zsh + oh-my-zsh**：`zsh-autosuggestions`、`zsh-syntax-highlighting`
- **oh-my-tmux（gpakosz/.tmux）** + **TPM**
- tmux 插件：`tmux-resurrect`（保存/恢复 session/window/pane）、`tmux-continuum`（每 15 分钟自动保存、启动时恢复）、`tmux-sensible`、`tmux-fzf`
- `t` = `tmux new -A -s main`（有则 attach，无则创建）
- **鼠标默认开启**：滚轮直接进入 copy-mode 滚动；`history-limit 200000`，`history-file ~/.tmux_history` 让历史跨重启保留；`allow-passthrough` 支持 yazi 图片预览
- session 状态持久化到 `~/.tmux/resurrect`，compose 已挂 named volume，重建容器不丢
- `direnv` 已接入 zsh，进入带 `.envrc` 的目录自动加载环境

### 新增的日常终端工具

`direnv`、`entr`（文件变化自动重跑）、`strace`、`tig`、`asciinema`、`rsync`、`pv`、`mtr`、`lsof`、`net-tools`、`bind9-dnsutils`、`nload`、`xxd`、`git-lfs`。完整建议见 [docs/TERMINAL_TOOLS.md](../docs/TERMINAL_TOOLS.md)。

### AI

镜像内已经固化：

- `~/.pi/agent/`：`settings.json`、`models.json`、extensions、agents、prompts、skills（**不含 auth.json**）
- `~/.pi/agent/mcp.json`：filesystem / mini / searxng 三个 MCP server
- `/opt/dotfiles.pi/`：MCP server、`pi-autorun.js`、`claude-autorun.mjs`

Pi：

```bash
export MERAAI_API_KEY=sk-...
pi -p "Reply with exactly: ok"
```

Claude Code：

```bash
export ANTHROPIC_AUTH_TOKEN=sk-...
export ANTHROPIC_BASE_URL=https://api.meraai.cn      # 按需
export ANTHROPIC_MODEL=deepseek-v4-flash
claude -p "Reply with exactly: ok"
```

> **ECC 插件**：镜像默认预装并启用 `ecc@ecc`（skills / agents / hooks），进入容器 `claude` 即可用。
> 代价（约 +35k 常驻 tokens、hooks 默认开启、约 +390MB 磁盘）、安装位置、验证与关闭方式见 **[ECC.md](ECC.md)**。
> 关闭预装：`INSTALL_ECC=0 ./docker/build.sh`；保留全部依赖：`ECC_PRUNE_DEPS=0 ./docker/build.sh`。
> 工具链与最佳实践（配置分层、权限、hooks、插件选型、C++/ROS2 工作流、排障）见 **[../docs/CLAUDE_TOOLCHAIN.md](../docs/CLAUDE_TOOLCHAIN.md)**。

nvim 内的 AI 默认关闭，需要时：

```bash
NVIM_ENABLE_AI=1 nvim
# CodeCompanion: <leader>ac / <leader>ai，需要 DEEPSEEK_API_KEY
# Copilot:      :Copilot auth
# sidekick:     驱动 Claude Code CLI
```

SearXNG（给 Pi 的 `searxng_search` 工具）：

```bash
# 用容器自带服务
docker compose -f docker/compose.yaml --profile search up -d searxng

# 或使用宿主机已有的 127.0.0.1:11199 服务
```

MCP 验证：

```bash
node /opt/dotfiles.pi/mcp-servers/mini-server.js <<< '{"jsonrpc":"2.0","id":1,"method":"initialize"}'
```

长程自动循环：

```bash
node /opt/dotfiles.pi/tools/pi-autorun.js "任务描述" 5 --out run.log
node /opt/dotfiles.pi/tools/claude-autorun.mjs "任务描述" 5 --out run.log
```

---

## 4. 目录与文件

| 路径 | 作用 |
| :--- | :--- |
| `docker/Dockerfile` | 镜像定义（ROS + C++ + Neovim + AI） |
| `docker/compose.yaml` | 开发服务、持久化卷、可选 SearXNG |
| `docker/build.sh` | 带命名构建上下文的构建入口 |
| `docker/dev.sh` | 填好路径并 `compose run` 的启动入口 |
| `docker/smoke.sh` | 容器内 C++/ROS2/Neovim/AI/CLI 自检脚本 |
| `docker/sync-vendor.sh` | 从本机 dotfiles 刷新 `vendor/` 快照 |
| `docker/vendor/` | CI/公开构建使用的非敏感配置快照 |
| `docker/Dockerfile.plus` | CI-only plus 镜像，`FROM` base 追加工具 |
| `docker/plus-install.sh` | plus 的 apt/pip/eget 工具安装脚本 |
| `.github/workflows/docker-image.yml` | base/vscode/plus 在线构建与推送 |
| `docker/entrypoint.sh` | 加载 ROS 环境、修正 git 安全目录 |
| `docker/claude.json` | Claude Code 首次启动的最小配置（无密钥） |
| `docker/mcp.json` | Pi 全局 MCP 配置（容器内路径） |
| `docker/gitconfig` | `delta` 分页器与 git 默认配置 |
| `docker/zshrc` / `bashrc` / `tmux.conf` | 容器内 shell/终端配置 |
| `docker/searxng/settings.yml` | 本地 SearXNG（开启 JSON API） |
| `docker/.env.example` | 环境变量模板 |

---

## 5. 设计要点

- **配置自包含**：非敏感的 nvim/Pi 配置快照 vendored 在 `docker/vendor/`，CI/公开构建直接使用；本机开发可用 `sync-vendor.sh` 或环境变量覆盖。
- **密钥不进镜像**：只复制 `settings.json` / `models.json` 等非敏感配置，`auth.json` 永远不复制；密钥在运行时通过环境变量注入。
- **同路径挂载**：`DOJO_DIR` 在宿主机和容器内保持一致，clangd、调试器和 `compile_commands.json` 都不需要改写路径。
- **可持久化**：`nvim-data` / `nvim-state` / `pi-sessions` / `claude-config` 是命名卷，重建镜像不丢插件历史、Pi 会话和 Claude 配置。
- **可复现**：Neovim 插件按 `lazy-lock.json` 恢复；Node/Pi/Claude/Neovim 版本都在构建参数里可覆盖。
- **已做缩容**：apt/npm/pip/Mason 缓存、`/tmp` 都在生成它们的同一层删除；需要更小镜像时用 `INSTALL_PREVIEW_DEPS=0` 去掉 ffmpeg/imagemagick/poppler/chafa/7zip。

---

## 6. 分发给学生

镜像本身已经包含 nvim/Pi 配置与插件，因此有两种用法：

1. **推荐：直接拉公开镜像**，学生不需要本地 dotfiles：
   ```bash
   docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
   ```
2. **学生本地构建**：仓库已 vendored 配置快照，直接 `./docker/build.sh` 即可；想用别人的配置时通过 `NVIM_CONFIG_DIR` / `PI_CONFIG_DIR` 覆盖。

无论哪种方式，都不要把 `~/.pi/agent/auth.json`、`~/.claude/` 或 `docker/.env` 打进镜像/仓库。

---

## 7. 常见问题

| 现象 | 处理 |
| :--- | :--- |
| `Cannot connect to the Docker daemon` | 先启动 Docker Desktop / `sudo service docker start` |
| `--build-context` 不识别 | 升级 Docker 23+，确认 BuildKit 启用：`DOCKER_BUILDKIT=1 docker build ...` |
| GitHub 下载慢/失败 | 设置 `HTTPS_PROXY` 后重建；或 `INSTALL_PLUGINS=0` 先用空插件启动 |
| 插件数量检查失败 | `./docker/build.sh` 时保证能访问 GitHub；或进容器执行 `:Lazy sync` |
| clangd 没有补全 | 进容器后重新 `python3 tools/train.py build`，确认工程路径与宿主机一致 |
| 文件变成 root 所有 | 用 `DEV_UID=$(id -u) DEV_GID=$(id -g) ./docker/build.sh` 重建 |
| TSan 在容器里报错 | 部分容器/虚拟化环境不允许 TSan，先确认是环境问题再怀疑代码 |
| rviz2/Gazebo 打不开 | 用 `BASE_IMAGE=osrf/ros:jazzy-desktop`，并挂载 X11/WSLg 的 `DISPLAY` 与 `/tmp/.X11-unix` |
| Pi 鉴权失败 | 检查 `MERAAI_API_KEY`；或把 `~/.pi/agent/auth.json` 只读挂载进容器 |
| Claude Code 首次启动要登录 | 设置 `ANTHROPIC_AUTH_TOKEN` + `ANTHROPIC_BASE_URL`，或进容器执行 `claude` 按提示登录 |

### 已知限制

- 公开构建使用 `docker/vendor/` 快照；它不包含 `auth.json`、`.env` 或真实 key。刷新快照后需要重新构建镜像。
- 桌面版基础镜像体积明显更大；只用 Stage 1 / Stage 2 纯 C++ 时不需要。
- `jless` / `json-tui` 只有 amd64 预编译，arm64 上自动跳过，用 `jnv` 代替。
- `yazi` 预览依赖较多；不需要图片/PDF/视频预览时可用 `INSTALL_PREVIEW_DEPS=0` 构建。
- 当前环境没有运行中的 Docker daemon，因此镜像未被实际构建验证；Dockerfile、compose、下载 URL 与二进制均已做静态检查。

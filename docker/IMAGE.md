# Docker 镜像记录

> 本地镜像信息 + 已发布 registry。每次 Dockerfile/vendor 变更后由 CI 重建并更新本文件。

## 主镜像：C++ / ROS 2 Jazzy / Neovim / AI

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:jazzy` |
| 本地 image id | `sha256:2f3d9c6cc99ee72a53a2534d5d70c57a8710aa44dc6ebb2963f19799509d4ebb` |
| 本地大小 | 6.7 GB（Mason 22 包，含 codelldb） |
| 构建日期 | 2026-10-26 |
| 冒烟测试 | `ALL CHECKS PASSED`（C++/ROS2/Neovim 82 插件/Mason 22 包/codelldb/CLI/Dojo/colcon/zsh/tmux） |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest`（匿名 manifest 已验证） |
| ACR digest | `sha256:2f3d9c6cc99ee72a53a2534d5d70c57a8710aa44dc6ebb2963f19799509d4ebb` |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest` |
| GHCR digest | `sha256:d29dae1b7ab4de30ae43e773610b4de62f29f5437a2982ce7eba5e71bfd294bf`（commit `5be8430`，含 tmux 鼠标/200k 历史） |
| 内容 | oh-my-zsh + autosuggestions/syntax-highlighting、oh-my-tmux、TPM、tmux-resurrect/continuum、direnv/entr/strace/tig/asciinema 等 |

## VS Code / code-server 变体

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:vscode`（当前 ACR 版 7.64 GB） |
| 基础镜像 | `debian:13.4` |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode`（匿名 manifest 已验证） |
| ACR digest | `sha256:3bfd1761b4871f33316d1a7c323ce18ec34ce444d3f0d0b7a07c771e102dfa4c`（不含 codelldb） |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode` |
| GHCR digest | `sha256:ff7de98691cecc5102795d554184366bf7ce4ebc395fdc771bcce58ba0dc0824`（commit `5be8430`，含 codelldb + tmux 鼠标/历史） |
| 说明 | code-server 4.143.1 + 18 个扩展 + C++ 工具链 + Neovim + AI + CLI；不含 ROS 2 Jazzy |

## Plus 版本（CI-only，依赖 base）

| 项 | 值 |
| :--- | :--- |
| 构建方式 | 仅 GitHub Actions `build-plus` job；**不在本地构建** |
| 依赖 | `FROM ghcr.io/...:sha-<commit>`（同一 commit 的 base） |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus` |
| GHCR digest | `sha256:a44fd522ea8ad51212f35d5ac70b8d6515b81073993dba00bcc951a7d959c5a6`（commit `5be8430`，含 uv/uvx + tmux 鼠标/历史） |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:plus`（设置 `ACR_ENABLED` 后 CI 同步；当前未推送） |
| 内容 | base + uv/uvx、lnav/iftop/nethogs/tcpdump/rclone/aria2/mosh/sshfs/glances/visidata、rr/pahole/gdb-dashboard、pre-commit/semgrep/gitleaks，以及 eget 安装的 atuin/mise/difftastic/broot/ouch/hexyl/zellij/dive/lazydocker/k9s/act/tailspin/bandwhich/csvlens/dasel/gron/mcfly/stern |
| 说明 | 工具安装 best-effort；`rga`（资产匹配失败）、`iwyu`（Ubuntu 源没有）、`bloaty`（上游无预编译）当前缺失，其余已装 |

## 拉取

```bash
# GHCR（public，已确认匿名可拉，无需 docker login）
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus

# 阿里云 ACR rapid7（已验证匿名 manifest 可读；较旧构建）
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode
```

> GHCR package 已设为 **Public**。验证结果：匿名 `docker manifest inspect` 对 `latest` / `vscode` / `plus` 均成功，外部无需登录。
>
> 设置入口（已完成，供以后参考）：
> <https://github.com/orgs/music-research-content/packages/container/package/cpp-ros2-engineering-dojo/settings>
> → Danger Zone → Change visibility → Public；如果 Public 被锁，先在 org settings → Code, planning, and automation → Packages 启用 Public package creation。
>
> ACR rapid7 的 `latest` 与 `vscode` 也匿名可读，但它们是较早构建（不含本轮 tmux 鼠标/200k 历史，也不含 uv）；plus 按约定只在 GHCR。最新 tmux/uv 版本请用 GHCR。

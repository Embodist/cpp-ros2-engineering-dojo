# Docker 镜像记录

> 本地镜像信息 + 已发布 registry。每次 Dockerfile/vendor 变更后由 CI 重建并更新本文件。

## 主镜像：C++ / ROS 2 Jazzy / Neovim / AI

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:jazzy` |
| 本地 image id | `sha256:5547eb609e951ce82f1a6045bb7b202aabcad563b2edc7459974a7098fa92d27` |
| 本地大小 | 6.11 GB（`docker images`，合并文件系统约 4.2 GB） |
| 构建日期 | 2026-10-26 |
| 冒烟测试 | `ALL CHECKS PASSED`（C++/ROS2/Neovim 82 插件/Mason/codelldb/CLI/Dojo/colcon/zsh/tmux） |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest`（匿名 manifest 已可读） |
| ACR digest | `sha256:5547eb609e951ce82f1a6045bb7b202aabcad563b2edc7459974a7098fa92d27` |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest`（CI 构建） |
| GHCR digest | 上一版 `sha256:d6b6060…`；本次含 zsh/tmux 的 CI 构建完成后更新 |
| 内容 | 另含 oh-my-zsh + autosuggestions/syntax-highlighting、oh-my-tmux、TPM、tmux-resurrect/continuum、direnv/entr/strace/tig/asciinema 等 |

## VS Code / code-server 变体

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:vscode` |
| 基础镜像 | `debian:13.4` |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode`（构建后推送） |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode`（CI 构建） |
| 说明 | code-server + C++ 工具链 + Neovim + AI + CLI；不含 ROS 2 Jazzy |

## 拉取

```bash
# GHCR（当前 package 默认 private；在 GitHub Package settings 改为 Public 后可匿名拉取）
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode

# 阿里云 ACR rapid7（已验证匿名 manifest 可读）
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode
```

> GHCR 的 package visibility 需要仓库管理员在 Package settings 里改为 Public（CI 的 GITHUB_TOKEN 没有改可见性的权限）。ACR rapid7 的 `latest` 已确认可以公开读取。

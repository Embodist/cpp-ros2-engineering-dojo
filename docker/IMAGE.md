# Docker 镜像记录

> 本地镜像信息 + 已发布 registry。每次 Dockerfile/vendor 变更后由 CI 重建并更新本文件。

## 主镜像：C++ / ROS 2 Jazzy / Neovim / AI

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:jazzy` |
| 本地 image id | `sha256:73434ad51dfb2885143d34a5dc5ba252dadfc82984ee0ba4c9e988df7f65e557` |
| 本地大小 | 6.04 GB（`docker images`，合并文件系统约 4.2 GB） |
| 构建日期 | 2026-10-26 |
| 冒烟测试 | `ALL CHECKS PASSED`（C++/ROS2/Neovim 82 插件/Mason/codelldb/CLI/Dojo/colcon） |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest` |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest` |

## VS Code / code-server 变体

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:vscode` |
| 基础镜像 | `debian:13.4` |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode` |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode` |
| 说明 | code-server + C++ 工具链 + Neovim + AI + CLI；不含 ROS 2 Jazzy |

## 拉取

```bash
# GHCR（公开）
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode

# 阿里云 ACR（如仓库为私有时先登录）
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode
```

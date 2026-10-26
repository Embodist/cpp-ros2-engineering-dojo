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
| GHCR digest | `sha256:4164541bbbeba111c323b17dd60c039812a9c1d025c12e464013102d6d954a4d`（commit `f086c07`） |
| 内容 | oh-my-zsh + autosuggestions/syntax-highlighting、oh-my-tmux、TPM、tmux-resurrect/continuum、direnv/entr/strace/tig/asciinema 等 |

## VS Code / code-server 变体

| 项 | 值 |
| :--- | :--- |
| 本地 tag | `cpp-ros2-dojo:vscode`（当前 ACR 版 7.64 GB） |
| 基础镜像 | `debian:13.4` |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode`（匿名 manifest 已验证） |
| ACR digest | `sha256:3bfd1761b4871f33316d1a7c323ce18ec34ce444d3f0d0b7a07c771e102dfa4c`（不含 codelldb） |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode` |
| GHCR digest | `sha256:f1d046377abfcc2fbb1436533a530dfcb08603cb7a483344c5b828b84af18cbd`（commit `f086c07`，含 codelldb） |
| 说明 | code-server 4.143.1 + 18 个扩展 + C++ 工具链 + Neovim + AI + CLI；不含 ROS 2 Jazzy |

## Plus 版本（CI-only，依赖 base）

| 项 | 值 |
| :--- | :--- |
| 构建方式 | 仅 GitHub Actions `build-plus` job；**不在本地构建** |
| 依赖 | `FROM ghcr.io/...:sha-<commit>`（同一 commit 的 base） |
| GHCR | `ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus` |
| GHCR digest | `sha256:db2d49047bcd196d481951253eb41f4bda4edbb5a788a997f45d1123466992d9`（commit `f086c07`） |
| ACR | `registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:plus`（设置 `ACR_ENABLED` 后 CI 同步；当前未推送） |
| 内容 | base + lnav/iftop/nethogs/tcpdump/rclone/aria2/mosh/sshfs/glances/visidata、rr/pahole/gdb-dashboard、pre-commit/semgrep/gitleaks，以及 eget 安装的 atuin/mise/difftastic/broot/ouch/hexyl/zellij/dive/lazydocker/k9s/act/tailspin/bandwhich/csvlens/dasel/gron/mcfly/stern |
| 说明 | 工具安装 best-effort；`rga`（资产匹配失败）、`iwyu`（Ubuntu 源没有）、`bloaty`（上游无预编译）当前缺失，其余已装 |

## 拉取

```bash
# GHCR（当前 package 默认 private；在 GitHub Package settings 改为 Public 后可匿名拉取）
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode

# 阿里云 ACR rapid7（已验证匿名 manifest 可读）
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest
docker pull registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:vscode

# plus 仅 CI 构建；GHCR 为最终来源，ACR 需开启 ACR_ENABLED 或手动同步
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus
```

> GHCR 的 package visibility 需要仓库管理员在 Package settings 里改为 Public（CI 的 GITHUB_TOKEN 没有改可见性的权限）。ACR rapid7 的 `latest` 与 `vscode` 均已确认可以匿名读取。
> ACR 的 `vscode` 当前是 7.64GB 的 pre-codelldb 版本；带 codelldb 的最终版本已由 CI 推到 GHCR（digest `33f583ce…`），本地拉取后会同步更新 ACR。

# Terminal 日常工具清单（C++ / ROS 2 开发向）

> 目标：少而精、启动快、可组合。分为**已内置**、**建议下一批**、**容器内慎用**三档。
> 对应镜像：`cpp-ros2-dojo:jazzy`（base，带 ROS 2）、`cpp-ros2-dojo:vscode`（code-server）、`ghcr.io/...:plus`（CI-only 扩容版）。
> plus 版本已内置下文 P1/P2 的大部分工具；plus 的安装清单见 [docker/README.md](../docker/README.md)。

## 1. 已内置

### 搜索 / 文件 / 文本
| 工具 | 用途 |
| :--- | :--- |
| `rg` (ripgrep) | 快速全文搜索 |
| `fd` | 快速找文件 |
| `fzf` | 模糊选择器（配合 fd/rg） |
| `bat` | 带高亮的 cat |
| `eza` | 带图标/ git 状态的 ls |
| `tree` | 目录树 |
| `yazi` | 终端文件管理器，带图片/PDF/视频预览 |
| `glow` | 终端渲染 Markdown |
| `xxd` | 十六进制查看 |
| `rsync` | 增量同步/备份 |
| `pv` | 管道进度条 |

### JSON / YAML / 数据
| 工具 | 用途 |
| :--- | :--- |
| `jq` | JSON 查询/变换 |
| `yq` | YAML/JSON 查询与改写 |
| `jnv` | 交互式 jq TUI |
| `jless` / `json-tui` | JSON 浏览器（amd64） |
| `sqlite3` | SQLite CLI |

### Git / 协作
| 工具 | 用途 |
| :--- | :--- |
| `lazygit` / `gitui` | Git TUI |
| `tig` | 轻量 Git 历史浏览 |
| `delta` | git diff 语法高亮、side-by-side |
| `gh` | GitHub CLI |
| `git-lfs` | 大文件（模型/点云/rosbag） |

### 进程 / 系统 / 磁盘
| 工具 | 用途 |
| :--- | :--- |
| `btop` / `btm` | 系统监控 TUI |
| `procs` | 现代 ps |
| `htop` | 经典进程监控 |
| `ncdu` / `dust` / `duf` | 目录/磁盘占用 |
| `lsof` | 打开的文件/端口 |
| `strace` | 系统调用跟踪 |
| `direnv` | 按目录加载环境变量 |
| `entr` | 文件变化后自动重跑命令 |
| `hyperfine` | 基准测试 |
| `asciinema` | 终端录制（适合留证据） |

### 网络
| 工具 | 用途 |
| :--- | :--- |
| `nload` | 实时网速 |
| `mtr` | ping + traceroute |
| `lsof -i` / `ss` | 端口与连接 |
| `dig` / `host` | DNS 诊断 |
| `xh` | 现代 HTTP 客户端（类似 httpie） |

### Shell / 终端
- **zsh + oh-my-zsh**：`zsh-autosuggestions`、`zsh-syntax-highlighting`
- **starship**：跨 shell 提示符
- **zoxide**：`z` 目录跳转
- **direnv hook**：自动加载目录级环境
- **tmux + oh-my-tmux（gpakosz/.tmux）**：好看且实用的 tmux 配置
- **TPM** + `tmux-resurrect` + `tmux-continuum` + `tmux-sensible` + `tmux-fzf`
  - `tmux-resurrect` 保存/恢复 session/window/pane
  - `tmux-continuum` 每 15 分钟自动保存，tmux 启动时恢复
  - 持久化目录：`~/.tmux/resurrect`（compose 已挂 named volume）
- 快捷命令：`t` = `tmux new -A -s main`（有则 attach，无则创建）
- tmux 默认鼠标滚动、`history-limit 200000`、`history-file` 持久化，`allow-passthrough` 支持图片预览

### C++ / ROS 2
- 工具链：`gcc`/`clang`/`clangd`/`clang-tidy`/`clang-format`/`cmake`/`ninja`/`ccache`
- 调试/分析：`gdb`/`lldb`/`valgrind`/`cppcheck`/`heaptrack`/ASan/TSan/UBSan
- ROS 2：`ros2` CLI、`colcon`、`rosdep`、`ros2 launch`（base 镜像）

## 2. 建议下一批（按价值/体积排序）

### P1：高频、体积小
| 工具 | 为什么值得 |
| :--- | :--- |
| `atuin` | SQLite 历史记录 + 跨 shell/跨机搜索，比 `Ctrl-R` 强很多 |
| `mise` | 按项目锁定 Node/Python/Rust/CMake 等版本 |
| `difftastic` | 语法感知的结构化 diff，C++ 模板/格式化改动更易读 |
| `lnav` | 日志分析 TUI（ROS 2 / CMake 长日志很实用） |
| `ouch` | 统一解压/压缩（zip/tar/7z/zst…） |
| `ripgrep-all` (`rga`) | 在 PDF/压缩包/Office 文档里搜 |
| `bandwhich` | 按进程看网络流量 |
| `iftop` / `nethogs` | 网络连接/进程流量 |
| `socat` / `netcat` | 端口转发、临时 TCP/UDP 调试 |
| `tcpdump` | 抓包（容器内需 `NET_ADMIN`/host 网络） |
| `gron` | JSON 扁平化成可 grep 的赋值 |
| `dasel` | 统一查询 JSON/YAML/TOML/XML |
| `hexyl` | 彩色 hex viewer |
| `broot` | 目录树 + 模糊跳转 |
| `thefuck` / `mcfly` | 上一条命令纠错 / 历史搜索 |
| `zellij` | tmux 的现代替代；支持 layout/session |
| `act` | 本地跑 GitHub Actions |
| `pre-commit` | Git hook 管理 |
| `gitleaks` / `semgrep` | 密钥/静态安全扫描 |
| `rclone` | 云存储同步（S3/OSS/Drive） |
| `aria2` | 多线程下载 |

### P2：C++ / ROS 2 专用
| 工具 | 为什么值得 |
| :--- | :--- |
| `include-what-you-use` (`iwyu`) | 清理头文件依赖，编译时间/耦合 |
| `bloaty` | 分析二进制体积（模板/调试信息） |
| `rr` | 录制/回放调试（需较新内核/权限） |
| `gdb-dashboard` | GDB TUI 增强 |
| `hotspot` | perf 数据可视化 |
| `pahole` | 结构体内存布局/对齐分析 |
| `ros2 bag` / `rosbag2` | 录包、回放、分析（base 镜像已含相关包） |
| `rqt` / `rviz2` / `foxglove` | 可视化，需要 desktop 镜像 + X11/WSLg |
| `ros2 doctor` / `ros2 topic hz` | 运行时诊断（ROS 2 CLI 已内置） |

### P3：容器 / 集群 / 运维
| 工具 | 说明 |
| :--- | :--- |
| `dive` | 分析镜像层与浪费空间 |
| `lazydocker` | Docker TUI（需要挂 docker.sock） |
| `k9s` / `kubectx` / `kubens` / `stern` | K8s 日常 |
| `mosh` / `sshfs` | 远程终端/文件 |
| `nmap` | 网络探测（不要在非授权网络批量扫） |

## 3. 容器内慎用 / 不适合

- `perf`、`bpftrace`、`tcpdump`、`nmap`：通常需要 `--cap-add`、host 网络或内核权限；在受限容器里会失败。
- `systemd` / `service`：容器里没有 systemd，服务请用进程管理或 compose。
- `rviz2` / Gazebo / `rqt`：需要 desktop 基础镜像 + X11/WSLg；纯 `ros-base` 不含 GUI。
- Docker-in-Docker / `lazydocker`：需要挂载 `/var/run/docker.sock`，等于把宿主机 Docker 权限交给容器，谨慎。
- GUI 类工具（浏览器、VS Code 桌面版）：用 code-server 或宿主机图形界面，不要在 base 镜像里堆 GUI。

## 4. 建议的添加方式

- 小工具优先 apt（Debian/Ubuntu 官方源），Rust 工具优先 GitHub 预编译；避免在镜像里 `cargo install` 全量编译。
- 新增工具放到**同一层**安装并清理缓存，避免镜像层膨胀。
- 需要更大集合时，建议做一个 `docker/Dockerfile.tools` 或在现有 Dockerfile 增加 `INSTALL_EXTRA_TOOLS=1` 开关，而不是默认全塞进基础镜像。
- 日常最小集建议：`rg fd bat eza fzf jq yq delta lazygit tig tmux zsh starship zoxide direnv entr strace rsync pv xxd`。

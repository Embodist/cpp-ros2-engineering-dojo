# C++ → ROS2 Engineering Dojo / C++ 工程训练营

> 一套 **题目 → 自己编码 → 测试 → 源码对照 → ROS2 项目** 的可执行训练仓库。核心题保留测试与参考实现；建议先自己做，再对照实现和 [Stage 1 手册](docs/STAGE1_SOLUTIONS.md)。

**适合对象**：有 Linux / Python / C++ 工作基础、C++ 久未动手、目标为 ROS2 二次开发/机器人中间件/Agent 安全研发的人。

**参考方式**：原创练习题和接口骨架，组织思路参考 C++ Core Guidelines 与《C++ 那些事》（CPlusPlusThings），没有复制后者的练习代码。所有外部参考见 [docs/REFERENCES.md](docs/REFERENCES.md)。

## 30 秒上手

```bash
# Ubuntu / WSL2 / Linux；需 Python 3、CMake >= 3.20、支持 C++20 的编译器
python3 tools/train.py list
python3 tools/train.py build                  # 只编译；当前仓库已含 01–12 参考实现
python3 tools/train.py test 01                # 当前应 PASS；练手时先合上参考实现
python3 tools/train.py test 01 --sanitize asan
python3 tools/train.py done 01                # 必须通过测试才能标记完成
python3 tools/train.py status

# 可选：GoogleTest 镜像套件（需要联网 FetchContent，或用 --gtest-source）
python3 tools/train.py test 01 --gtest
ctest --test-dir build-gtest -L gtest --output-on-failure
```

等价 CMake 命令：
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R '^dojo_01_' --output-on-failure

# GoogleTest 可选路径
cmake -S . -B build-gtest -DDOJO_USE_GTEST=ON
cmake --build build-gtest --parallel 2
ctest --test-dir build-gtest -L gtest --output-on-failure
# 离线：加 -DDOJO_GTEST_SOURCE_DIR=/path/to/googletest
```

`ctest` 全量现在应通过（01–12 已实现）；如果你回退成空骨架，失败才是预期。不要把“能编译”视为完成。

第一次使用建议按 [docs/ONBOARDING.md](docs/ONBOARDING.md) 的 3 天快速上手执行；完整阅读方法见 [docs/READING_AND_PROGRESSION.md](docs/READING_AND_PROGRESSION.md)。

## 测试：离线 harness + 可选 GoogleTest

- **默认路径（离线）**：`stage1/support/test_harness.hpp` + `python3 tools/train.py test <ID>`，不依赖 GoogleTest/Catch2，不联网。
- **可选路径（生产级）**：`-DDOJO_USE_GTEST=ON` 构建 `stage1/tests_gtest/` 的 79 项 GoogleTest 镜像测试，使用 `Tracer`/`ThrowOnN`/`MoveOnly`/`NoexceptToggle`/`LeakCheck`/`NewCounter` 夹具，注册进 CTest。
- WSL 下 TSan 自动加 `setarch ... -R` launcher。
- 完整命令、夹具用法、离线 googletest 配置见 [docs/TESTING.md](docs/TESTING.md)。
- **项目线参考测试**：`python3 tools/train.py project p01 --test`；全量 `ctest --test-dir build-projects -L projects`。详见 [docs/TESTING.md](docs/TESTING.md)。

```bash
python3 tools/train.py test 01 --gtest
ctest --test-dir build-gtest -L gtest --output-on-failure
```

## 统一开发容器（推荐）

仓库自带 Docker 环境：**C++20 + ROS 2 Jazzy + Neovim(LazyVim) + Pi/Claude Code/MCP/SearXNG**，以及 `rg`/`fd`/`bat`/`eza`/`jq`/`jnv`/`jless`/`json-tui`/`yazi`/`btop` 等现代 CLI/TUI。

前提：Docker 23+。仓库已 vendored nvim/Pi 配置快照，无需本机 dotfiles；公开镜像：

```bash
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
```

```bash
./docker/dev.sh                          # 进入容器（首次自动构建）
./docker/dev.sh python3 tools/train.py list
./docker/dev.sh nvim .
./docker/dev.sh bash -lc 'cd stage2/ros2_ws && colcon build --symlink-install'
```

VS Code / code-server 变体（Debian 13.4，不含 ROS 2）：

```bash
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:vscode
docker compose -f docker/compose.yaml --profile vscode up -d vscode
# http://localhost:8080，默认密码 user
```

阿里云 ACR rapid7 空间也有同名镜像：`registry.cn-chengdu.aliyuncs.com/rapid7/cpp-ros2-engineering-dojo:latest` / `:vscode`。

**plus 版本**（更多终端/C++/DevOps 工具）只由 CI 从 base 镜像在线构建，不在本地 build：`ghcr.io/music-research-content/cpp-ros2-engineering-dojo:plus`，详见 [docker/README.md](docker/README.md)。

密钥、ROS 2、X11、SearXNG 与故障排查见 [docker/README.md](docker/README.md)，最新镜像记录见 [docker/IMAGE.md](docker/IMAGE.md)，终端工具清单见 [docs/TERMINAL_TOOLS.md](docs/TERMINAL_TOOLS.md)。容器只固化配置，不固化密钥。

## 两阶段结构

- **Stage 1（训练题）**：`stage1/cards/` 共有 **60 个核心任务卡**，按语言基础 / 所有权 / STL / 接口 / 并发 / 工程排障六大类组织；另有 `stage1/extensions/` 的 **5 个可选扩展任务**（模板/concepts、STL 深水区、C++20、内存模型、测试/CI）。其中 **12 道核心题有 C++20 代码骨架和本地可执行测试**，其余是要求自己新建文件、做实验、写解释与性能报告的任务。
- **Stage 2（组件实战）**：`stage2/components/` 共有 **10 份需求规格**，提供接口边界、验收用例、故障注入、设计评审问题和交付物，不提供答案。`stage2/ros2_ws/` 附最小 ROS2 C++ 包骨架；本机无 ROS2 时不影响 Stage 1 构建。
- **项目线（可选）**：把 `stage1/projects` 的 p01–p13 系统项目接到 Stage 1 先修题与 Stage 2 组件；最小闭环 p01 v1→v2/v3 → p02a → p05 → p08 → p09 → p10 → p11 → p12 → p13。见 [stage1/PROJECT_TRACK.md](stage1/PROJECT_TRACK.md)。
- **源码精读**：`source_reading/` 给出 LevelDB / STL / ROS2 `rclcpp` 阅读路径与实验证据要求；读源码不等于读完文件，必须落到可验证的调用链和不变量。
- **上手手册**：`docs/ONBOARDING.md` 给出 3 天快速上手、8 周路线、每题工作流、工具链速查和故障排查。
- **文档阅读与进阶**：`docs/READING_AND_PROGRESSION.md` 给出权威文档分层、cppreference 阅读 SOP、分阶段材料清单与练习路径。
- **覆盖矩阵**：`docs/CORE_GUIDELINES_MAP.md` 按强/中/弱/未覆盖列出 Core Guidelines 分区，并标出 5 个扩展任务的补强位置。
- **规范/评估/训练法**：`docs/` 有团队风格规范、8 周安排、评分标准、AI 陪练词、调试操作手册。

快速查找全部题目：[60 题核心 + 5 扩展](stage1/TASK_INDEX.md)；项目线：[PROJECT_TRACK](stage1/PROJECT_TRACK.md)；扩展说明：[stage1/extensions/README.md](stage1/extensions/README.md)；第二阶段：[组件导航](stage2/README.md)。

## 推荐完成顺序

1. **基础手感**：01、02、03、04、05、06、07；每天做 2～3 张不带代码骨架的任务卡。
2. **并发与工程**：08、09、10、11、12；用 ASan/UBSan/TSan 检查（TSan 另行构建，不能与 ASan 混用）。
3. **源码对照**：手写 MiniVector 后对照库实现；写并发队列后追 ROS2 Executor；LevelDB 选一条 Put/Get 路径。
4. **组件实战**：先 Stage 2 的 C01～C04，再 ROS2 C06、C07、C08。对 ROS2 更重要的路径优先。
5. **按缺口补强**：从 [E01–E05](stage1/extensions/README.md) 选 1–2 个扩展，优先补模板/concepts、STL 深水区、C++20 ranges/expected、内存模型或测试/CI。
6. **项目线（可选）**：把 `stage1/projects` 的 p01–p13 作为 capstone（参考实现已同步），先修题与顺序见 [PROJECT_TRACK](stage1/PROJECT_TRACK.md)。

## 验收门槛

- 自动测试通过仅代表可见测试通过，**不代表完全正确**。
- 每题写 `notes/<ID>.md`：画对象所有权/线程/数据流图，说明边界和失败路径；提交一个失败复现及修复证据。
- 对含并发、内存的题跑对应 Sanitizer；含性能题对照基准、单位和测试环境。
- 最后以“脱离 AI、依据文档从空文件重新实现核心模块”为保持手感的最低标准。

## 不包含

- 默认构建不依赖 GoogleTest、Catch2 或联网下载；附自制轻量测试运行器。可选的 GoogleTest 镜像套件通过 `-DDOJO_USE_GTEST=ON` 启用（FetchContent 固定 v1.15.2，可指向本地源码），不改变默认离线路径。
- 项目线参考实现已同步到 `stage1/projects/reference/`（可选构建）；默认构建不依赖第三方项目源码。本仓库不代表官方 Core Guidelines 内容，也不保证覆盖特定企业面试题库。
- ROS2 可选骨架要求你自行在安装了 ROS2 的机器编译（或使用 [docker/](docker/) 中的 Jazzy 容器）；这里未假装验证过 ROS2 环境。

从 [START_HERE.md](START_HERE.md) 开始。许可证见 [LICENSE](LICENSE)。

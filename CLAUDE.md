# CLAUDE.md — C++ / ROS2 Engineering Dojo

训练仓库：题目 → 自己编码 → 测试 → 源码对照 → ROS2 项目。**核心实现故意留空**，不要把它当答案库或“坏掉了”。

## 关键命令

```bash
python3 tools/train.py list          # 列出任务
python3 tools/train.py build         # 编译（未实现任务也能编译通过）
python3 tools/train.py test 03       # 跑某个任务的测试（初始应为 FAIL）
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py status
python3 tools/train.py done 03       # 仅当测试通过才可标记完成
```

等价 CMake：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R '^dojo_03_' --output-on-failure
```

ROS 2（Stage 2）：

```bash
cd stage2/ros2_ws
colcon build --symlink-install --packages-select robot_dojo_pipeline
source install/setup.bash
```

容器内使用：`./docker/dev.sh ...`（详见 docker/README.md；镜像已含 ECC 插件，见 docker/ECC.md）。

## 约定

- 语言标准 **C++20**；格式化/静态检查用仓库根的 `.clang-format` / `.clang-tidy`。
- 不要改动 `tools/train.py`、测试判分逻辑或任务骨架的接口来“骗过”测试。
- `ctest` 全量初始失败是**预期**；初始只有 `dojo_smoke` 应通过。
- 编译产物走 `build/`（已忽略）；`compile_commands.json` 供 clangd 使用。

## 目录

- `stage1/`：C++ 基础/工程题；`stage2/`：ROS 2 项目；`tools/`：训练器；`docs/`：参考资料。

## 注意

- 不要提交 `.env`、`~/.pi/agent/auth.json`、`~/.claude/`、任何密钥。
- 修改 Dockerfile/compose 后需重建镜像验证：`./docker/build.sh`，进容器 `bash docker/smoke.sh`。

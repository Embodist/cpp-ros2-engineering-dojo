# C++ → ROS2 Engineering Dojo / C++ 工程训练营

> 一套 **题目 → 自己编码 → 测试 → 源码对照 → ROS2 项目** 的可执行训练仓库。核心实现故意留空；这不是答案库。

**适合对象**：有 Linux / Python / C++ 工作基础、C++ 久未动手、目标为 ROS2 二次开发/机器人中间件/Agent 安全研发的人。

**参考方式**：原创练习题和接口骨架，组织思路参考 C++ Core Guidelines 与《C++ 那些事》（CPlusPlusThings），没有复制后者的练习代码。所有外部参考见 [docs/REFERENCES.md](docs/REFERENCES.md)。

## 30 秒上手

```bash
# Ubuntu / WSL2 / Linux；需 Python 3、CMake >= 3.20、支持 C++20 的编译器
python3 tools/train.py list
python3 tools/train.py build                  # 只编译，未实现的任务仍能编译
python3 tools/train.py test 01                # 预期最初 FAIL（这是训练题）
# 修改 stage1/include/dojo/scope_exit.hpp 后重复运行
python3 tools/train.py test 01
python3 tools/train.py done 01                # 必须通过测试才能标记完成
python3 tools/train.py status
```

等价 CMake 命令：
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R '^dojo_01_' --output-on-failure
```

`ctest` 全量最初会失败，这是**预期**；只有 `dojo_smoke` 应在初始状态通过。不要把空骨架视为完成或误以为仓库损坏。

## 两阶段结构

- **Stage 1（训练题）**：`stage1/cards/` 共有 **60 个独立任务卡**，按语言基础 / 所有权 / STL / 接口 / 并发 / 工程排障六大类组织。其中 **12 道核心题有 C++20 代码骨架和本地可执行测试**，其余是要求自己新建文件、做实验、写解释与性能报告的任务。
- **Stage 2（组件实战）**：`stage2/components/` 共有 **10 份需求规格**，提供接口边界、验收用例、故障注入、设计评审问题和交付物，不提供答案。`stage2/ros2_ws/` 附最小 ROS2 C++ 包骨架；本机无 ROS2 时不影响 Stage 1 构建。
- **源码精读**：`source_reading/` 给出 LevelDB / STL / ROS2 `rclcpp` 阅读路径与实验证据要求；读源码不等于读完文件，必须落到可验证的调用链和不变量。
- **规范/评估/训练法**：`docs/` 有团队风格规范、Core Guidelines 对照、8 周安排、评分标准、AI 陪练词、调试操作手册。

快速查找全部题目：[60 题导航](stage1/TASK_INDEX.md)；第二阶段：[组件导航](stage2/README.md)。

## 推荐完成顺序

1. **基础手感**：01、02、03、04、05、06、07；每天做 2～3 张不带代码骨架的任务卡。
2. **并发与工程**：08、09、10、11、12；用 ASan/UBSan/TSan 检查（TSan 另行构建，不能与 ASan 混用）。
3. **源码对照**：手写 MiniVector 后对照库实现；写并发队列后追 ROS2 Executor；LevelDB 选一条 Put/Get 路径。
4. **组件实战**：先 Stage 2 的 C01～C04，再 ROS2 C06、C07、C08。对 ROS2 更重要的路径优先。

## 验收门槛

- 自动测试通过仅代表可见测试通过，**不代表完全正确**。
- 每题写 `notes/<ID>.md`：画对象所有权/线程/数据流图，说明边界和失败路径；提交一个失败复现及修复证据。
- 对含并发、内存的题跑对应 Sanitizer；含性能题对照基准、单位和测试环境。
- 最后以“脱离 AI、依据文档从空文件重新实现核心模块”为保持手感的最低标准。

## 不包含

- 不依赖 GoogleTest、Catch2、第三方库联网下载；附自制轻量测试运行器，**不主张用它替代生产级测试框架**。
- 本仓库不附成熟项目源码，不代表官方 Core Guidelines 内容，也不保证覆盖特定企业面试题库。
- ROS2 可选骨架要求你自行在安装了 ROS2 的机器编译；这里未假装验证过 ROS2 环境。

从 [START_HERE.md](START_HERE.md) 开始。许可证见 [LICENSE](LICENSE)。

# 上手手册：3 天跑通，8 周进阶

> 面向有 Linux / Python / C++ 基础、C++ 久未动手、目标 ROS2 二次开发 / 机器人中间件 / Agent 安全的人。
> 配套：[阅读指南](READING_AND_PROGRESSION.md)、[8 周路线](ROADMAP_8_WEEKS.md)、[覆盖矩阵](CORE_GUIDELINES_MAP.md)、[调试手册](DEBUGGING.md)、[题目导航](../stage1/TASK_INDEX.md)。

## 0. 先选路径

| 路径 | 适合谁 | 做什么 | 完成标准 |
|---|---|---|---|
| 3 天快速上手 | 先想确认环境和训练方式 | 01 → 02/03 → 08/09，跑通“编码→测试→工具→笔记”循环 | 能有证据地通过至少 3 题 |
| 8 周主线 | 想系统恢复手感并做 ROS2 组件 | 按 [8 周路线](ROADMAP_8_WEEKS.md) 完成 01–12 + 选做 13–44 + Stage2 | 有 notes/evidence，能脱 AI 复写核心机制 |
| 单题循环 | 时间碎片化 | 每题按第 5 节的 6 个动作执行 | 当题测试/证据通过 |
| 扩展补强 | 核心题做完、发现覆盖缺口 | E01–E05（[扩展索引](../stage1/extensions/README.md)） | 按扩展卡验收 |

**不要先看教程 3 小时。** 先跑一题，遇到问题再查文档。

## 1. 环境检查

```bash
g++ --version && cmake --version && python3 --version
python3 tools/train.py list
```

要求：CMake >= 3.20、GCC >= 11 / Clang >= 14、Python 3；使用 C++20。没有 ROS2 不影响 Stage 1 构建。

不想手动配环境：

```bash
docker pull ghcr.io/music-research-content/cpp-ros2-engineering-dojo:latest
./docker/dev.sh
./docker/dev.sh python3 tools/train.py list
```

容器细节见 [docker/README.md](../docker/README.md)。

## 2. 第 1 天：跑通 01

```bash
python3 tools/train.py build
python3 tools/train.py test 01          # 预期 FAIL，这是训练题
python3 tools/train.py show 01
```

然后：

1. 读 `stage1/cards/01-scope-exit.md`，在 `notes/01.md` 写下预测：所有权、不变量、异常路径、边界。
2. 修改 `stage1/include/dojo/scope_exit.hpp`，不要先让 AI 给完整实现。
3. 自己新增至少 2 个反例测试。
4. 重跑：

```bash
python3 tools/train.py test 01
python3 tools/train.py done 01
```

**初始 FAIL 不是仓库坏了**，是故意留空的 TODO。可见测试通过也不等于完全正确，必须写 `notes/01.md`。

## 3. 第 2 天：02/03 + 工具链

```bash
python3 tools/train.py test 02
python3 tools/train.py test 03
python3 tools/train.py test 03 --sanitize asan
```

- 02：句柄 RAII，重点看 `release/reset/move` 的关闭次数。
- 03：MiniVector，重点看 allocator、placement construction、强异常保证、move-only 元素。
- 用 gdb 至少走一次栈：

```bash
gdb --args build/stage1/dojo_test_03_mini_vector
(gdb) run
(gdb) bt
(gdb) frame 0
(gdb) print this->size_
```

交付：`notes/02.md`、`notes/03.md`；03 的 ASan 输出和一次失败注入记录。

## 4. 第 3 天：并发 08/09 + TSan

```bash
python3 tools/train.py test 08 --sanitize tsan
python3 tools/train.py test 09 --sanitize tsan
```

重点不是“跑通”，而是能解释：

- 队列关闭与 `push/pop` 的所有竞争；
- 条件变量为什么必须用谓词；
- ThreadPool 析构时如何停止并 join；
- TSan 报什么、不报什么。

TSan 在部分 WSL / 容器 / 内核映射上可能启动失败；先分清工具环境和代码问题，再决定是否换原生 Linux 重测。若并发还不熟，第 3 天可先做 13/14，再回到 08/09。

## 5. 每题工作流：6 个动作 + 五问卡

1. **预测**：在笔记写出行为、不变量、边界和错误处理。
2. **编码**：只查标准库 API 文档，自己写核心算法；卡 20–40 分钟再求助。
3. **验证**：跑可见测试，并新增至少 2 个反例；打开编译器警告。
4. **工具**：生命周期/内存题跑 ASan+UBSan；并发题单独跑 TSan；性能题做基准。
5. **评审**：让 AI 找缺陷但不直接改代码；你提交修复和设计解释。
6. **复现**：24–72 小时后新建空文件，脱离 AI 复写核心机制。

每次查 API，回答五问卡：

- 复杂度是多少？
- 异常保证是什么？
- 哪些迭代器/引用/指针会失效？
- 有哪些前置条件？违反是否 UB？
- 什么场景不该用它？

详细阅读方法见 [阅读指南](READING_AND_PROGRESSION.md)。

## 6. 目录与交付物

| 内容 | 位置 |
|---|---|
| 01–12 骨架实现 | `stage1/include/dojo/` |
| 13–60 开放实验 | `stage1/labs/<ID>/` |
| E01–E05 扩展实验 | `stage1/labs/E01/` … `E05/` |
| 每题复盘 | `notes/<ID>.md` |
| Stage2 组件 | `stage2/work/Cxx/` |
| 组件设计/证据 | `stage2/templates/DESIGN.md`、`evidence.md` |

开放实验至少交付：`main.cpp`（或 `include/`+`src/`）、`README.md`、`evidence.md`。`evidence.md` 必须写命令、实际输出、反例、未覆盖风险，不能只写“已验证”。

```bash
python3 tools/train.py lab 13
python3 tools/train.py done 13 --manual   # 需要 evidence.md 且不能有 TODO
```

扩展任务同理：

```bash
python3 tools/train.py show E01
python3 tools/train.py lab E01
python3 tools/train.py done E01 --manual
```

## 7. 工具链速查

```bash
# 构建 / 测试
python3 tools/train.py build
python3 tools/train.py test 01
python3 tools/train.py status

# Sanitizer
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py test 08 --sanitize tsan

# 等价 CMake
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel 2
ctest --test-dir build -R '^dojo_01_' --output-on-failure

# 调试
gdb --args build/stage1/dojo_test_03_mini_vector

# 性能（先定工作负载，再 Release 构建）
perf stat ./your_benchmark
```

工具细节见 [调试手册](DEBUGGING.md)。TSan 与 ASan 不能同时启用。

## 8. 8 周路线速览

| 周 | 重点 | 核心训练 | 退出门槛 |
|---|---|---|---|
| 1 | RAII、生命周期、移动 | 01–04 + 13–28 选 4 | 03 可用于非平凡类且无明显泄漏 |
| 2 | 接口、解析、工程 | 05–07 + 29–44 选 4 | 能说明所有权与错误通道 |
| 3 | 并发、锁、任务 | 08–12 + 45–52 选 4 | 能解释 close/shutdown 的所有竞争 |
| 4 | 性能、源码 | 11 + 53–60 选 4 | ASan/TSan + 一份性能记录 |
| 5 | 组件 I | C01–C03 | API + 构建 + 错误路径 + 自动测试 |
| 6 | ROS2 基础 | C06 | 3 节点流水线跑通 |
| 7 | ROS2 并发/诊断 | C07–C08 | 可重复实验数据 |
| 8 | 融合项目/作品集 | C09 | 脱 AI 从空骨架完成一个小功能 |

完整说明见 [8 周路线](ROADMAP_8_WEEKS.md)。扩展任务按缺口选修，不挤占主线。

## 9. 证据清单与自评

每题至少留下：

- 编译/测试/调试/Sanitizer 命令和实际输出；
- 至少 2 个自己新增的反例；
- 所有权图、线程图或调用链图（按题目类型）；
- 性能题的硬件、编译器、优化级别、数据规模和重复次数；
- 未解决问题，以及 24–72 小时复写结果。

自评按 [SCORING.md](SCORING.md)：5 个维度各 0–4 分，建议 >=16/20 且正确性 >=3；内存安全/并发致命问题一票否决。

## 10. 常见故障排查

| 现象 | 处理 |
|---|---|
| `test 01` 初始失败 | 预期行为；骨架故意留 TODO |
| 找不到 cmake/g++ | 安装工具链，或用 `./docker/dev.sh` |
| TSan 启动失败 | 常见于 WSL/容器映射；记录环境，换原生 Linux 重测 |
| 构建目录混乱 | 新建独立构建目录（如 `build-asan`），不要删除别人的构建产物 |
| 没有 ROS2 | Stage 1 不受影响；Stage2 C06–C08 用 Jazzy 容器 |
| AI 直接给完整实现 | 用 [AI 陪练提示词](AI_COACH_PROMPTS.md) 约束为分层提示 |
| 测试通过但讲不清 | 回到五问卡和 24–72 小时复写；测试通过不等于掌握 |

## 11. 下一步

1. 先完成 01–12 核心骨架。
2. 从 13–44 中按缺口选做，至少覆盖所有权、STL/迭代器、错误处理、并发。
3. 走 Stage2 主线：**C01 → C03 → C06 → C07 → C09**。
4. 按 [覆盖矩阵](CORE_GUIDELINES_MAP.md) 选 1–2 个扩展：E01（模板/concepts）、E02（STL 深水区）、E03（ranges/expected）、E04（内存模型）、E05（测试/CI）。
5. 最后做一次“无 AI 从空文件复写”验收，并整理作品集 README、架构图和性能日志。

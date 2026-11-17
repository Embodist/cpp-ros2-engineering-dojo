# Stage 1 全题手册（60 核心 + 5 扩展）

> 这是参考手册，不是替代练习。01–12 的代码已经写入 `stage1/include/dojo/` 并通过自动测试；13–60 和 E01–E05 是开放实验，给的是目标、参考实现思路、关键代码/实验、验收证据和常见坑。建议先自己写一版，再对照手册。

## 内容范围

| 范围 | 形式 | 状态 |
|---|---|---|
| 01–12 核心可执行题 | 完整参考实现 + 测试推演 + 边界 | 已实现并验证 |
| 13–60 开放实验 | 每题目标、思路、关键片段、验收清单 | 攻略已写 |
| E01–E05 扩展 | 每题前置、思路、关键片段、验收 | 攻略已写 |

## 阅读顺序

1. [01–04 ScopeExit / UniqueHandle / MiniVector / MoveOnlyBuffer](stage1_handbook/01-04.md)
2. [05–08 Pipeline / Packet / ParseResult / BoundedQueue](stage1_handbook/05-08.md)
3. [09–12 ThreadPool / RingBuffer / MovingAverage / CallbackRegistry](stage1_handbook/09-12.md)
4. [13–28 语言核心与对象生命周期](stage1_handbook/13-28.md)
5. [29–44 STL、接口与错误处理](stage1_handbook/29-44.md)
6. [45–60 并发、性能、工程与 ROS2 概念](stage1_handbook/45-60.md)
7. [E01–E05 扩展](stage1_handbook/extensions.md)

## 已验证状态

普通测试与 ASan/UBSan：

| 题号 | 测试结果 | ASan/UBSan |
|---|---|---|
| 01 ScopeExit | 7/7（含两个反例） | 通过 |
| 02 UniqueHandle | 3/3 | 通过 |
| 03 MiniVector | 5/5 | 通过 |
| 04 MoveOnlyBuffer | 2/2 | 通过 |
| 05 Pipeline | 3/3 | 通过 |
| 06 BinaryPacket | 5/5 | 通过 |
| 07 ParseResult | 3/3 | 通过 |
| 08 BoundedQueue | 3/3 | 通过 |
| 09 ThreadPool | 3/3 | 通过 |
| 10 RingBuffer | 2/2 | 通过 |
| 11 MovingAverage | 2/2 | 通过 |
| 12 CallbackRegistry | 4/4 | 通过 |

TSan：

- 08：直接通过。
- 09、12：WSL 下直接运行报 `ThreadSanitizer: unexpected memory mapping`；用 `setarch $(uname -m) -R ./build-tsan/stage1/dojo_test_09_thread_pool`（12 同理）后通过。这是环境限制，不是代码竞争。
- GoogleTest 镜像套件：79/79 在普通、ASan/UBSan、TSan 下通过；12 的并发压测在 gtest 套件中。

## 常用命令

> 测试有两条路径：默认离线 harness，以及可选的 GoogleTest 镜像套件。完整说明见 [TESTING.md](TESTING.md)。

```bash
# 单题测试
python3 tools/train.py test 01
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py test 08 --sanitize tsan

# 可选 GoogleTest 镜像套件
python3 tools/train.py test 01 --gtest
ctest --test-dir build-gtest -L gtest --output-on-failure

# 查看题目/进度
python3 tools/train.py show 01
python3 tools/train.py status

# 开放实验目录与手动完成
python3 tools/train.py lab 13
python3 tools/train.py done 13 --manual
```

## 目录结构

```text
docs/
  STAGE1_SOLUTIONS.md          # 本文件：总目录
  stage1_handbook/
    01-04.md                   # 核心可执行题完整实现
    05-08.md
    09-12.md
    13-28.md                   # 开放实验攻略
    29-44.md
    45-60.md
    extensions.md              # E01–E05

stage1/include/dojo/           # 01–12 参考实现
stage1/cards/                  # 原题卡片
stage1/labs/                   # 13–60 / E01–E05 交付目录
notes/                         # 每题学习记录
```

## 使用建议

1. 读一题的“目标与不变量”，先在 `notes/<ID>.md` 写出自己的预测。
2. 合上手册，从空文件重写核心机制；卡住 20–40 分钟再看参考实现。
3. 跑测试、加至少 2 个反例、跑对应 sanitizer。
4. 把命令和真实输出写进 `notes/<ID>.md` 或 `stage1/labs/<ID>/evidence.md`。
5. 24–72 小时后脱稿复写；讲不清的地方就是下一步重点。
6. 13–60 是开放题，没有唯一答案；手册的片段只展示核心机制，完整项目、README 和证据仍需自己完成。
7. 想做系统项目：按 [stage1/PROJECT_TRACK.md](stage1/PROJECT_TRACK.md) 的先修题 → capstone 顺序；项目代码在 `stage1/projects/pXX`，证据写到 `notes/projects/pXX.md`。

## 项目线（已同步）

- p01–p11 参考实现/测试已同步到 `stage1/projects/reference/`；工作区在 `stage1/projects/pXX/`。
- 单项目：`python3 tools/train.py project p01 --test`；全量：`ctest --test-dir build-projects -L projects`。
- 已验证：plain 147/147；ASan/UBSan 147/147；TSan 并发项目 30/30。
- 映射、先修题和能力链见 [stage1/PROJECT_TRACK.md](stage1/PROJECT_TRACK.md)。

## 注意

- 手册里的代码是参考实现，不是唯一写法；重点是能解释不变量、所有权、异常保证和失效规则。
- 13–60 的片段可能需要按卡片补齐头文件、错误处理和测试；不要直接当作可提交项目。
- 58/59 涉及 ROS2，手册只给概念映射和实验设计，不假设本机已安装 ROS2。

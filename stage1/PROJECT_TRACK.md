# Stage 1 项目线（可选）：p01–p13 已同步到 Stage 1

> **60 题是单项技术动作，p01–p13 是完整系统项目。** 项目线不替代 60 题，而是把项目作为阶段性 capstone：先用 dojo 的先修题把动作练对，再用项目验收反向加固 Stage 1。
>
> 项目代码已经同步到 Stage 1：工作区在 `stage1/projects/pXX`，参考实现在 `stage1/projects/reference/pXX`。参考实现用于对照和跑测试，不要照抄；自己的实现写在工作区。

## 0. 项目 CLI

```bash
python3 tools/train.py projects            # 全部项目：状态、优先级、先修缺口
python3 tools/train.py projects --minimal  # 只看最小闭环
python3 tools/train.py project p01         # 查看细节
python3 tools/train.py project p01 --start # 生成项目卡/检查表/3 次计划/证据模板，标记 WIP
python3 tools/train.py project p01 --done  # 证据无 TODO 后标记 PASS
python3 tools/train.py project p01 --test  # 构建/运行移植后的参考测试
python3 tools/train.py project p08 --test --sanitize tsan
```

项目参考实现的 CMake 开关：`DOJO_BUILD_PROJECTS=ON`（要求 `DOJO_USE_GTEST=ON`）。可用 presets `projects`、`projects-asan`、`projects-tsan`，或直接 `ctest --test-dir build-projects -L projects`。

`--start` 会生成：

```text
stage1/projects/p01/README.md      # 项目卡：先修、里程碑、能力承接/输出
stage1/projects/p01/CHECKLIST.md   # 先修、工具、证据、复盘
stage1/projects/p01/PLAN.md        # 3 次起跑计划
notes/projects/p01.md              # 证据（--done 检查这里没有 TODO）
```

`projects` 会根据任务状态和项目依赖提示下一步；项目状态存放在 `.dojo_projects.json`（已 gitignore）。

## 1. 两条课程怎么配合

| | dojo Stage 1 | 项目线（已同步） |
|---|---|---|
| 形态 | 60 个单项动作 + 12 个自动测试骨架 | 13 个完整系统项目 |
| 强项 | 测试、边界、sanitizer、ROS2 组件、每题可验证 | 库/系统设计、所有权、分配器、无锁、实时、数学 |
| 用法 | 先做 drill，拿到单项能力 | 再做 capstone，把能力组装成系统 |
| 验收 | `notes/<ID>.md` + 测试/工具证据 | `stage1/projects/pXX/README.md` 核查点 + `stage1/projects/docs/功能要求清单.md` |

建议节奏：**每个项目先完成映射的 2–4 道先修题，再开项目；项目验收时把证据写进 `notes/projects/pXX.md`。**

## 2. 总映射表

| 项目 | 阶段 | 先修 Stage 1 题 | 对应 Stage 2 | 项目核心 |
|---|---|---|---|---|
| p01 学生管理系统 v1–v3 | P1 语义与所有权 | 01, 02, 05, 07, 13–17, 20–28, 29–36（选） | C01, C02 | 值语义、所有权、多态、异常安全 |
| p02 UniquePtr / SharedPtr / WeakPtr | P2 资源管理 | 01, 02, 13, 17, 21, 23, 24, 26, 34, 37 | C03, C04 | RAII、五法则、控制块、原子计数、自定义 deleter |
| p03 String（SSO） | P2 资源管理 | 03, 04, 13–16, 21, 22, 26, 27, 29–36, 38 | C05, C10 | 五法则、SSO、别名安全、string_view |
| p04 List\<T> | P3 容器与泛型 | 03, 20, 21, 22, 29–36, 37, 38 | C10 | 节点所有权、迭代器、splice/sort |
| p05 Vector\<T, Alloc> | P3 容器与泛型 | 03, 10, 19, 29–36, 38 | C01, C10 | 原始内存、强异常、move_if_noexcept、迭代器 |
| p06 Arena / 对象池 | P3 容器与泛型 | 03, 10, 49, 53, 54, 56 | C01, C04, C10 | allocator、对齐、池、零 malloc |
| p07 Function / MoveOnlyFunction | P4 抽象与并发 | 05, 12, 17, 37, 38, 43 | C03, C04 | 类型擦除、SBO、callable 生命周期 |
| p08 BlockingQueue / ThreadPool | P4 抽象与并发 | 08, 09, 45–52 | C03, C04, C07 | mutex/CV、关闭、异常、future |
| p09 SPSC 无锁环形队列 | P4 抽象与并发 | 08, 10, 45–52 | C04, C07 | atomic、memory order、false sharing |
| p10 迷你 pub/sub + Executor | P5 机器人综合 | 05, 12, 39, 43, 52, 58, 59 | C03, C06, C07 | 回调生命周期、shared_ptr、executor、timer |
| p11 零分配实时控制环 | P5 机器人综合 | 08–11, 49, 50, 56, 57 | C04, C07 | 零分配、零锁、周期抖动 |
| p12 小型数学库 | P5 机器人综合 | 11, 19, 30, 35, 36, 38, 56 | C08 | constexpr、数值稳定、内存布局 |
| p13 ROS2 综合落地 | P5 机器人综合 | 58, 59, 60 + p10–p12 | C06, C07, C08, C09 | 组合节点、intra-process、无分配 |

## 3. 推荐 capstone 子集

不要把 13 个项目全部塞进 8 周。按 `stage1/projects/docs/推进清单.md` 的 P0 优先序选：

| 优先级 | 项目 | 理由 |
|---|---|---|
| P0-1 所有权 | p01 v2 + p02a | 所有权图、控制块、移动后状态 |
| P0-2 内存模型 | p09 | atomic / happens-before 最容易写错 |
| P0-3 异常安全 | p01 v3 + p05 | ThrowOnN 扫描每个抛出点 |
| P0-4 测量 | p08 / p10 / p11 | sanitizer、压力测试、抖动数据 |
| P1-1 类型擦除 | p07 | SBO、callable 生命周期 |
| P1-2 并发工程 | p08 | 关闭/背压/异常传播 |
| P2 可选 | p03, p04, p06 | 按缺口选；p12 是 p13 前置，做收官时必做 |

最小闭环：**p01 v1→v2/v3 → p02a → p05 → p08 → p09 → p10 → p11 → p12 → p13**。

## 4. 8 周项目线（可选，扩展）

主线仍是 [8 周路线](../docs/ROADMAP_8_WEEKS.md)。如果要做项目线，可以这样插入；时间会比纯 drill 紧，建议按 10–15 小时/周或延长到 10–12 周。

| 周 | Stage 1 drills | 项目 capstone | Stage 2 |
|---|---|---|---|
| 1 | 01–04 + 13/17/21 | p01 v1 | — |
| 2 | 05–07 + 22/23/25/26 | p01 v2/v3 + p02a | — |
| 3 | 08–12 + 45–52 选 4 | p05 核心 + p08 | — |
| 4 | 29–36 选 4 + 53–60 选 4 | p09 + p10 起步 | C01/C03 |
| 5 | — | p10 完成 + p11 起步 | C06/C07 |
| 6 | — | p11 完成 | C07/C08 |
| 7 | — | p12 | C08/C09 |
| 8 | — | p13 | C09/C10 |

可选插入：p02b（控制块/weak_ptr）、p03（SSO）、p04（链表迭代器）、p06（池分配器）、p07（类型擦除）按缺口补。

> p01 v2/v3 以 v1 为前提；p05 v0–v2 可在 03 后开始，v3/v4（insert/emplace/erase/迭代器）放到 29–36 之后。

## 5. 逐项目集成细节

### p01 学生管理系统 v1–v3

- 先修：01/02（RAII）、05（多态）、07（错误通道）、13–17（值类别/初始化/view/lambda）、20–28（所有权/异常安全）。
- 节奏：01–04 后做 v1；05/07/21–28 后做 v2；22 + ThrowOnN 后做 v3。
- 用 dojo 夹具：`Tracer` 证明查询/排序无多余拷贝；`ThrowOnN` 扫描批量导入；`LeakCheck` 检查析构平衡。
- 验收：`stage1/projects/p01/README.md` 的 v1/v2/v3 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 1。
- Stage 2：C01（数据管道）、C02（设备生命周期）。

### p02 UniquePtr / SharedPtr / WeakPtr

- 先修：01/02（RAII）、13/17/21/24/26（值类别/五法则/工厂/移动）、23（循环引用）、34（optional）、37（类型擦除）。
- 节奏：p02a 在 01/02/13/24/26 后；p02b 在 23/37/E04 后。
- 用 dojo 夹具：`Tracer` 跟踪控制块/对象析构；`NewCounter` 证明 `MakeShared` 少一次分配；TSan 验证并发计数。
- 验收：p02 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 2；写出控制块布局与释放时序。
- Stage 2：C03（事件总线所有权）、C04（调度器生命周期）。

### p03 String（含 SSO）

- 先修：03（原始内存）、04（缓冲区）、13–16（值类别/view/span）、21/22（五法则/异常安全）、26/27（移动/悬空）、29–36（迭代器/算法）、38（模板）。
- 用 dojo 夹具：`NewCounter` 证明短串零分配；`Tracer` 跟踪拷贝/移动；差分测试对照 `std::string`。
- 验收：p03 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 3；10 万步差分一致。
- Stage 2：C05（流式解码）、C10（MiniKV）。

### p04 List\<T>

- 先修：03（allocator/生命周期）、20（结构化绑定）、21/22（五法则/异常安全）、29–36（迭代器/失效）、37/38（模板/concepts）。
- 用 dojo 夹具：`MoveOnly` 证明只走移动；`Tracer` 证明 `sort` 零元素拷贝；差分测试对照 `std::list`。
- 验收：p04 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 4。
- 扩展：E02（STL 深水区）；Stage 2 C10。

### p05 Vector\<T, Alloc>

- 先修：03（旗舰前置）、10（环形缓冲）、19（constexpr）、29–36（迭代器/算法）、38（模板）。
- 用 dojo 夹具：`ThrowOnN` 扫描扩容/插入/拷贝的每个抛出点；`NoexceptToggle` 验证 `move_if_noexcept`；`Tracer` 证明构造数等于析构数；差分测试对照 `std::vector`。
- 验收：p05 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 5；能不看资料手写 v1 核心（扩容 + 析构）。
- 扩展：E02/E03；Stage 2 C01、C10。

### p06 Arena / 对象池

- 先修：03（allocator）、10（容量/布局）、49（false sharing）、53/54（CMake/符号）、56（perf）；建议先完成 p05。
- 用 dojo 夹具：`NewCounter` 证明运行期零 malloc；ASan 检查对齐/越界；benchmark 对比 `std::allocator`。
- 验收：p06 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 6；接入 p05/p04 后差分测试仍通过。
- Stage 2：C01、C04、C10。

### p07 Function / MoveOnlyFunction

- 先修：05（接口/策略）、12（回调注册表）、17（lambda 捕获）、37（类型擦除）、38（模板）、43（多态组合）。
- 用 dojo 夹具：`Tracer` 跟踪 capture 拷贝/移动；`NewCounter` 证明小 callable 零分配、大 callable 一次分配。
- 验收：p07 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 7；能说清虚函数版 vs 函数指针表版取舍。
- 扩展：E01；Stage 2 C03、C04。

### p08 BlockingQueue / ThreadPool

- 先修：08（有界队列）、09（线程池）、45–52（jthread/死锁/CV/原子/背压/future/重入）。
- 用 dojo 夹具：`MoveOnly` 作任务类型；TSan；多生产者/多消费者压力测试。
- 验收：p08 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 8；TSan 与压力测试连续跑 100 次无失败。
- 扩展：E04；Stage 2 C03、C04、C07。

### p09 SPSC 无锁环形队列

- 先修：08/10（队列/环形缓冲）、45–52（原子/内存序/false sharing/背压）。
- 用 dojo 夹具：`MoveOnly` 验证非平凡类型；TSan；有/无 padding 吞吐对比。
- 验收：p09 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 9；能画出 happens-before 链并解释每个 memory order。
- 扩展：E04；Stage 2 C04、C07。

### p10 迷你 pub/sub + Executor

- 先修：05（接口）、12（回调生命周期）、39（const/线程安全）、43（多态组合）、52（重入）、58（QoS 概念）、59（executor 回调组）。
- 用 dojo 夹具：`Tracer`/`LeakCheck` 验证消息生命周期；TSan 验证多线程 executor；`shared_ptr<const Msg>` 零拷贝。
- 验收：p10 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 10；能口述 publish → 回调的完整路径。
- Stage 2：C03、C06、C07。

### p11 零分配实时控制环

- 先修：08–11（队列/线程/均值）、49（false sharing）、50（背压）、56（perf）、57（时间戳）；建议先完成 p06、p09。
- 用 dojo 夹具：`NewCounter` 证明运行期零分配；`perf`/直方图记录 p50/p99/max 抖动。
- 验收：p11 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 11；解释每处“为什么不能分配/加锁”。
- 扩展：E04；Stage 2 C04、C07。

### p12 小型数学库

- 先修：11（数值稳定）、19（constexpr）、30（严格弱序）、35（ranges）、36（二分边界）、38（模板）、56（perf）。
- 用 dojo 夹具：`static_assert` 编译期测试；与已知值/Eigen 对比；`is_standard_layout`/`is_trivially_copyable` 断言。
- 验收：p12 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 12；运动学例子与数值微分一致。
- Stage 2：C08（TF2/时间同步）。

### p13 ROS2 综合落地

- 先修：58（QoS）、59（executor）、60（CLI/日志）+ p10–p12；建议先完成 Stage 2 C06/C07。
- 用 dojo 夹具：`NewCounter` 验证回调无分配；带 `Tracer` 的消息类型验证 intra-process 拷贝下降。
- 验收：p13 README 核查点 + `stage1/projects/docs/功能要求清单.md` 项目 13；`ros2 run`/`topic hz`/`topic delay` 数据。
- Stage 2：C06、C07、C08、C09。

## 6. 测试、夹具与证据

- 夹具复用：`stage1/support/common/` 的 `Tracer`、`ThrowOnN`、`MoveOnly`、`NoexceptToggle`、`LeakCheck`、`NewCounter`。
- GoogleTest：项目里的测试可以照 `docs/TESTING.md` 的双轨方式构建；dojo 的 `stage1/tests_gtest/` 是 01–12 的镜像，不是项目测试。
- 参考测试在 `stage1/projects/reference/pXX/tests`；工作区测试写 `stage1/projects/pXX/tests`；用 `project pXX --test` 跑参考测试。
- 证据落点：`notes/projects/pXX.md`（模板见 `notes/projects/_template.md`），每条核查点写测试名/命令/原始输出。
- 代码规则：项目代码自己写；AI 只做 review、最小复现和边界提问（与 cpp_testing 的“只教练不代写”一致）。

## 7. 与 Stage 2 的衔接

- p01 → C01/C02：数据管道与设备生命周期。
- p02/p08/p10 → C03/C04：所有权、并发、退出语义。
- p05/p06 → C01/C10：容器与分配器。
- p09/p11 → C04/C07：无锁/实时/executor 延迟。
- p12 → C08：位姿与 TF2。
- p13 → C06/C07/C08/C09：ROS2 综合。

## 8. 边界

- 项目线是**可选扩展**，不改变 60 题主线和 8 周骨架。
- 不把 p01–p13 全部塞进 8 周；优先 P0 子集。
- 参考实现已同步到 `stage1/projects/reference`；工作区 `pXX` 由你自己写，不直接改 reference；上游 cpp_testing 只作为来源记录。
- 如果以后要把项目并进 dojo，再单独建 `stage1/projects/` 脚手架；当前只做映射和证据接口。

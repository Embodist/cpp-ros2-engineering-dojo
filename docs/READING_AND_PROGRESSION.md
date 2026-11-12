# 文档阅读与进阶：权威资料、阅读 SOP、练习路径

> 目标：把“查文档”变成可重复的工程动作，把“进阶”变成可验收的练习。
> 配套文档：[上手手册](ONBOARDING.md)、[外部参考](REFERENCES.md)、[8 周路线](ROADMAP_8_WEEKS.md)、[调试工作流](DEBUGGING.md)、[Core Guidelines 覆盖矩阵](CORE_GUIDELINES_MAP.md)、[源码精读](../source_reading/README.md)。

**最短路径**：第一次使用先看 [上手手册](ONBOARDING.md) 的 3 天快速上手；然后读第 2 节的五问卡，立刻拿一道题用一遍；卡住时按第 4 节找材料；做完按第 5 节判断是否升级。

## 1. 权威文档分层：先选对资料，再开始读

不要把一种文档用于所有问题。按问题类型选层级：

| 层级 | 资料 | 权威性 | 用于 | 不要用于 |
|---|---|---|---|---|
| L1 规范 | [ISO C++ 工作草案](https://eel.is/c++draft/)；正式标准 ISO/IEC 14882 | 公开的规范工作草案；正式标准为最终依据 | UB、对象生命周期、重载决议、语言规则争议 | 入门教程 |
| L2 事实参考 | [cppreference（英）](https://en.cppreference.com/w/cpp) / [中文镜像](https://zh.cppreference.com/w/cpp) | 社区维护的事实参考，非规范原文 | 日常查 API：签名、复杂度、异常保证、失效规则 | 从头通读；替代标准条款 |
| L3 工程守则 | [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) | 工程实践权威 | 设计评审、所有权、接口、并发风格 | 当作语言法律；逐条教条化 |
| L4 实现验证 | [Compiler Explorer](https://godbolt.org/)、[编译器支持表](https://en.cppreference.com/w/cpp/compiler_support) | 工具输出 | 验证标准理解、比较实现差异、看代码生成 | 代替文档阅读 |
| L5 设计动机 | [WG21 提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/) | 一手设计材料 | 理解“为什么有这条规则/特性” | 第一遍学习 C++ |
| L6 项目文档 | [ROS 2 Docs](https://docs.ros.org/)、[REPs](https://ros.org/reps/rep-0000.html)、rclcpp API | 官方项目文档 | Executor、QoS、rclcpp、ament_cmake | 跨发行版盲用 |
| L7 系统材料 | Stroustrup《A Tour of C++》、Josuttis《C++17/20》、Meyers 系列、Williams《C++ Concurrency in Action》 | 系统化但非规范 | 建立框架、查漏补缺 | 当作最终裁决 |

选择规则：

- **事实问题**查 L2；**规则争议/UB** 查 L1。
- **该不该这样设计**查 L3；**实现是否一致**查 L4。
- **为什么这样设计**查 L5；**ROS 2 行为**查 L6。
- 书是路线图，不是裁决书；关键结论回到 L1/L2 核对。

### 引用纪律

- cppreference 要记录**适用标准版本**（页面上的 C++11/17/20 标记）。
- 标准草案要记录**条款编号**（如 `[basic.life]`），不要凭记忆引用。
- ROS 2 要记录**发行版**（如 Jazzy），不要拿 Rolling 文档直接套。
- 源码要记录**仓库 commit SHA**，不要引用会漂移的行号。
- 性能结论要记录编译器、优化级别、硬件、数据规模和重复次数。

## 2. cppreference 阅读 SOP（每次 20–40 分钟）

cppreference 是检索型手册，不是教程。固定按下面步骤读：

### 2.1 带着一个问题进入

先用一句话写出问题，例如：“`std::vector::push_back` 在什么情况下使迭代器失效？异常时有什么保证？”

### 2.2 固定阅读顺序

1. 页面标题、头文件、标准版本标记。
2. **Synopsis / 声明**：先看签名和重载，不逐字读正文。
3. **Parameters / Return**：类型约束、返回值语义。
4. **Complexity**：O(1)、均摊 O(1) 还是 O(n)。
5. **Exceptions**：抛什么；强/基本/不抛异常保证。
6. **Notes / 特殊规则**：迭代器、引用、指针失效；前置条件；是否 UB。
7. **Example**：抄最小示例，本地编译验证。
8. **See also / Related**：只追与当前问题直接相关的链接。

### 2.3 五问卡：读完必须能回答

- [ ] 复杂度是多少？最坏和均摊是否不同？
- [ ] 异常保证是什么？失败后对象处于什么状态？
- [ ] 哪些迭代器/引用/指针会失效？何时失效？
- [ ] 有哪些前置条件？违反是否 UB？
- [ ] 什么场景**不该**用它？替代方案是什么？

任何一项答不出，就记录为“未解问题”，不要猜。

### 2.4 验证，而不是感觉

- 把示例复制到临时文件，用 `-Wall -Wextra -Wpedantic` 编译。
- 涉及生命周期/越界/UB 的代码，跑 ASan+UBSan。
- 涉及实现差异的代码，放 [Compiler Explorer](https://godbolt.org/) 对比 GCC/Clang。
- 涉及语言规则的争议，查到草案条款并记录编号。

### 2.5 产出到 `notes/<ID>.md`

阅读必须留下可复查的产物，模板见第 7 节。

### 2.6 24–72 小时后脱稿复述

合上文档，用自己的话重讲五个问题，并重写最小示例。讲不出来的部分就是没学会的部分。

## 3. 其他文档的读法

### 3.1 ISO C++ 草案

- 用页面内搜索或索引定位，不按顺序通读。
- 区分规范正文、Note、Example：Note 和 Example 不是规范要求。
- 记录条款编号，回到代码和测试验证，不凭条文想象行为。
- 只查当前问题相关的最小范围，读完回到任务。

### 3.2 C++ Core Guidelines

- 每条规则看 **Rule + Reason + Example + Enforcement**。
- 记录你实际核对过的规则编号，不要杜撰编号。
- 结合 [Core Guidelines 对照](CORE_GUIDELINES_MAP.md) 映射到训练题。
- 规则是工程建议，不是所有项目都必须逐条执行；说清取舍理由。

### 3.3 ROS 2 文档

- 先确认发行版，再打开对应版本的 Concepts / How-to / API。
- 阅读顺序：概念（Executor、Callback Group、QoS）→ How-to → API 参考。
- 行为问题优先看设计文档和 REP，不只看 API 签名。
- 每个结论配一个可重复实验：节点数量、QoS、线程数、时间戳、原始输出。

### 3.4 源码

- 先写下要回答的问题，再打开文件；不要“读完一个仓库”。
- 记录 commit SHA，画调用链图，列出不变量。
- 做一次失败注入，交付最小复现代码。
- 至少记录一个“我原先预测错了”的点。

### 3.5 书

- 一次只读与当前任务相关的一章，读前写下问题。
- 每章至少落一个练习或一个可运行示例。
- 书的结论与 cppreference/标准冲突时，以 L1/L2 为准并记录差异。

## 4. 材料清单（按阶段）

| 阶段 | 必读材料 | 选读/扩展 | 对应训练 | 交付物 |
|---|---|---|---|---|
| L0 工具链 | GCC/Clang 警告选项、CMake 入门、GDB 手册、Sanitizer 文档 | Valgrind、clang-tidy | 01–03 | 构建日志 + 一次 sanitizer 抓错记录 |
| L1 语言核心 | cppreference：RAII、智能指针、移动语义、异常安全；Core Guidelines R/C 类 | 《A Tour of C++》语言框架章节 | 01–07 + 13–28 中选 4 | 每题 `notes/<ID>.md` |
| L2 STL、接口与泛型 | cppreference：容器、迭代器、算法、模板、concepts；[STL vector 精读](../source_reading/01-STL-VECTOR.md) | libstdc++/libc++ vector 源码 | 29–44 中选 4 | MiniVector 源码对照记录 + 4 份实验笔记 |
| L3 并发与性能 | cppreference：thread/mutex/condition_variable/atomic/memory_order；TSan 文档 | 《C++ Concurrency in Action》相关章节、`perf` | 08–12 + 45–52 中选 4 | ThreadPool + TSan 报告 + 基准记录 |
| L4 工程与源码 | CMake 文档、clang-tidy、[LevelDB 精读](../source_reading/02-LEVELDB.md)、相关 WG21 提案 | CI 配置、依赖管理 | 11 + 53–60 中选 4（58–59 可并入 L5） | 调用链图 + 不变量表 + 失败注入 |
| L5 ROS 2 | Concepts、Executors、Callback Groups、QoS、ament_cmake、rclcpp API、TF2 | REPs、ros2_control | Stage 2 C01–C08；可合并 58–59 | 3 节点流水线 + 可复现实验记录 |

**L0 工具直达**：

- [GCC Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html) / [Clang Diagnostics Reference](https://clang.llvm.org/docs/DiagnosticsReference.html)
- [CMake Documentation](https://cmake.org/cmake/help/latest/)
- [GDB Documentation](https://sourceware.org/gdb/documentation/)
- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)、[UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)、[ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)

**可选扩展（按缺口选修）**：

| ID | 主题 | 补强 | 前置 |
|---|---|---|---|
| E01 | 变参模板、SFINAE 与 concepts | T / Con | 03、38 |
| E02 | STL 深水区：list、rb_tree、traits、EBO | SL / T | 03、29–36 |
| E03 | C++20 ranges 与 expected | SL / ES / E | 03、07、35 |
| E04 | 并发内存模型：happens-before 与 memory_order | CP | 08–12、45–52 |
| E05 | 测试、CI 与包管理 | SF / T | 03 或 08 |

训练题索引：[60 题核心 + 5 扩展](../stage1/TASK_INDEX.md)；扩展说明：[extensions/README](../stage1/extensions/README.md)；组件需求：[Stage 2](../stage2/README.md)。

## 5. 进阶阶梯：每级都有退出门槛

不达到门槛不升级。L0 与 L1 可并行，其余按顺序推进；周次与 [8 周路线](ROADMAP_8_WEEKS.md) 对齐，但不要求严格串行。周期按每天 4–6 小时估算。

### Level 0：工具链闭环（1–2 周）

- **阅读**：警告选项、CMake 构建、GDB、ASan/UBSan/TSan。
- **练习**：用 `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` 构建；gdb 看栈；对一个生命周期 bug 跑 ASan。
- **退出门槛**：能独立读懂警告并用 sanitizer 证明一个真实 bug。

### Level 1：语言核心（第 1–2 周）

- **阅读**：RAII、值语义、所有权、Rule of Zero/Five、移动语义、异常安全；const/值类别、初始化、`string_view`/`span` 生命周期、lambda 捕获。
- **练习**：01–07，并从 13–28 中选 4 道；每题按 [START_HERE](../START_HERE.md) 的 6 个动作执行。
- **退出门槛**：不看答案写出非平凡类，无泄漏，能解释所有权与异常保证。

### Level 2：STL、接口与泛型（第 2–3 周）

- **阅读**：容器、迭代器、算法、模板、concepts；手写 MiniVector 后对照 STL（03）。
- **练习**：29–44 中选 4 道；记录失效规则和异常保证。可选：E01（模板/concepts）、E02（STL 深水区）。
- **退出门槛**：能预测修改容器操作对迭代器/引用的影响。

### Level 3：并发与性能（第 3–4 周）

- **阅读**：线程、锁、条件变量、原子、内存序；TSan 与基准方法。
- **练习**：08–12 + 45–52 中选 4 道；ThreadPool 对照 ROS 2 Executor。可选：E04（内存模型）、E03（ranges/expected）。
- **退出门槛**：能解释 close/shutdown 的所有竞争，给出可重复的性能数据。

### Level 4：工程与源码（第 4 周起）

- **阅读**：多 target CMake、clang-tidy、LevelDB 一条调用链、相关设计提案。
- **练习**：11 + 53–60 中选 4 道（58–59 可并入 L5）；完成一次失败注入。可选：E05（测试/CI/包管理）。
- **退出门槛**：能从源码定位行为、预测修改影响、用测试证明。

### Level 5：ROS 2 工程（第 5–8 周）

- **阅读**：Executor、Callback Group、QoS、生命周期、TF2、ament_cmake。
- **练习**：Stage 2 C01–C08；可合并 58–59；先无 ROS 依赖组件，再 ROS 2 流水线。
- **退出门槛**：能搭 3 节点流水线，解释并发/QoS 行为并复现实验。

## 6. 每日与每周执行模板

### 每日（4–6 小时）

1. 10 分钟：写下今天要回答的 1–3 个文档问题。
2. 90 分钟：核心实现，尽量不看答案。
3. 60–90 分钟：测试、调试、加反例。
4. 45–60 分钟：源码阅读或标准/指南查阅。
5. 20–40 分钟：卡住时查文档，上限 40 分钟。
6. 10 分钟：更新 `notes/<ID>.md`，记录未解问题。

### 每周

- 一次 45–90 分钟“无 AI”验收：从空文件写一个新变种。
- 一次笔记审查：把本周结论压缩成五问卡，删掉复制粘贴的原文。
- 一次引用核对：随机抽 3 条结论，回到 cppreference/草案/官方文档验证。
- 一次工具检查：ASan/TSan/基准按题目需要跑一遍，记录环境。

## 7. 阅读笔记模板

复制到 `notes/<ID>.md`：

```markdown
# <任务 ID> <主题>

## 问题
用一句话写清要回答什么。

## 结论（自己的话）
不用复制文档原文，写出可复述的结论。

## 最小示例
可编译、可运行的最小验证代码。

## 边界与反例
前置条件、失败路径、我新增的反例测试。

## 五问卡
- 复杂度：
- 异常保证：
- 失效/生命周期规则：
- 前置条件/UB：
- 不该用的场景：

## 引用
- cppreference URL + 适用标准版本：
- 标准草案条款编号：
- 源码 commit SHA / ROS 2 发行版：

## 未解问题
明确写下仍不确定的点，下一次先查这里。

## 24–72 小时复述
合上文档后重讲的结果；讲不清的地方就是下一步重点。
```

## 8. 进阶自测：用证据判断，不用时长判断

- [ ] 看到一个 API，能预测复杂度、异常保证和失效规则。
- [ ] 看到编译错误，能在一分钟内判断方向，而不是从第一行往下读。
- [ ] 遇到 bug 先写测试复现，再改代码。
- [ ] 能用 sanitizer 或基准给出证据，而不是“我觉得没问题”。
- [ ] 能脱 AI 从空文件重写核心机制。
- [ ] 能画出所有权/线程/调用链图，并解释失败路径。
- [ ] 能给出标准条款编号、ROS 2 发行版或源码 commit SHA 作为依据。

## 9. 常见误区

- **收藏等于学会**：没有笔记、示例和复述，链接没有价值。
- **从头读 cppreference**：它是检索手册，按任务查。
- **只看中文页**：中文可快速定位，关键结论回英文核对。
- **只查 API 不看 UB/失效规则**：这是 C++  bug 的高发区。
- **测试通过就算完成**：可见测试通过不等于正确，必须写反例和笔记。
- **先看答案或让 AI 代写**：会写答案不等于建立手感。
- **混用 ROS 2 版本**：Rolling 文档不保证适用于你安装的发行版。
- **读源码不记 commit**：路径和行号会漂移，结论无法复查。
- **性能结论没有环境和单位**：没有测量条件的数字不可信。

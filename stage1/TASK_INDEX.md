# 60 题核心导航 + 5 扩展 / Task Navigator

**01–12** 有头文件 TODO + 本地自动测试；**13–60** 是实验题，要自建实现和证据。**E01–E05** 是可选扩展，用于补核心题未系统覆盖的缺口。

| ID | 类别 | 题目 | 规范映射 |
|---|---|---|---|
| 01 | 可执行骨架 | [ScopeExit 与异常安全 RAII](cards/01-scope-exit.md) | R.1, C.30, C.31, C.66 |
| 02 | 可执行骨架 | [UniqueHandle：句柄 RAII](cards/02-unique-handle.md) | R.1, R.5, C.21, C.66 |
| 03 | 可执行骨架 | [MiniVector<T> 内存与异常安全](cards/03-mini-vector.md) | R.1, R.10, C.20, C.21, C.66 |
| 04 | 可执行骨架 | [MoveOnlyBuffer：零拷贝所有权](cards/04-move-buffer.md) | R.3, R.11, F.24, C.66 |
| 05 | 可执行骨架 | [Stage 接口与策略组合](cards/05-polymorphic-pipeline.md) | C.35, I.11, R.34 |
| 06 | 可执行骨架 | [二进制帧安全编解码](cards/06-binary-packet.md) | I.1, ES.47, F.24 |
| 07 | 可执行骨架 | [结构化错误与无异常解析](cards/07-parse-result.md) | E.2, E.6, I.12 |
| 08 | 可执行骨架 | [可关闭的有界阻塞队列](cards/08-bounded-queue.md) | CP.20, CP.22, CP.42, CP.43 |
| 09 | 可执行骨架 | [ThreadPool：任务执行与关停](cards/09-thread-pool.md) | CP.22, CP.25, CP.26, CP.43 |
| 10 | 可执行骨架 | [固定容量环形缓冲区](cards/10-ring-buffer.md) | R.11, Per.1, C.20 |
| 11 | 可执行骨架 | [滑动均值：数值稳定与输入约束](cards/11-moving-average.md) | P.9, F.4, E.6 |
| 12 | 可执行骨架 | [CallbackRegistry：重入与生命周期](cards/12-callback-registry.md) | CP.22, I.11, R.34 |
| 13 | 开放实验 | [const 与值类别](cards/13-practice.md) | F.16, Con.2 |
| 14 | 开放实验 | [初始化与窄化](cards/14-practice.md) | ES.23 |
| 15 | 开放实验 | [string_view 生命周期](cards/15-practice.md) | F.24, R.3 |
| 16 | 开放实验 | [span 与数组边界](cards/16-practice.md) | F.24, I.1 |
| 17 | 开放实验 | [Lambda 捕获与所有权](cards/17-practice.md) | F.53, CP.31 |
| 18 | 开放实验 | [enum class 与状态机](cards/18-practice.md) | Enum.3, I.12 |
| 19 | 开放实验 | [constexpr 与编译时约束](cards/19-practice.md) | P.9 |
| 20 | 开放实验 | [结构化绑定与所有权](cards/20-practice.md) | Per.1 |
| 21 | 开放实验 | [Rule of Zero 对照实验](cards/21-practice.md) | C.20, C.21 |
| 22 | 开放实验 | [异常安全分级](cards/22-practice.md) | E.12, R.1 |
| 23 | 开放实验 | [引用计数循环](cards/23-practice.md) | R.24 |
| 24 | 开放实验 | [工厂返回资源](cards/24-practice.md) | R.11, R.12 |
| 25 | 开放实验 | [继承析构陷阱](cards/25-practice.md) | C.35 |
| 26 | 开放实验 | [std::move 并非移动](cards/26-practice.md) | C.66 |
| 27 | 开放实验 | [临时对象引用悬挂](cards/27-practice.md) | F.43 |
| 28 | 开放实验 | [pImpl 与 ABI 边界](cards/28-practice.md) | I.27 |
| 29 | 开放实验 | [unordered_map 的失效规则](cards/29-practice.md) | SL.con.1 |
| 30 | 开放实验 | [排序与严格弱序](cards/30-practice.md) | P.4 |
| 31 | 开放实验 | [deque vs vector 数据通路](cards/31-practice.md) | Per.1 |
| 32 | 开放实验 | [哈希碰撞实验](cards/32-practice.md) | Per.1 |
| 33 | 开放实验 | [迭代器失效实战](cards/33-practice.md) | SL.con.1 |
| 34 | 开放实验 | [optional vs magic number](cards/34-practice.md) | I.12 |
| 35 | 开放实验 | [投影与 ranges](cards/35-practice.md) | SL.con.1 |
| 36 | 开放实验 | [二分搜索边界](cards/36-practice.md) | P.9 |
| 37 | 开放实验 | [类型擦除回调](cards/37-practice.md) | T.1, F.16 |
| 38 | 开放实验 | [模板错误可读性](cards/38-practice.md) | T.10 |
| 39 | 开放实验 | [const 接口与线程安全](cards/39-practice.md) | CP.2 |
| 40 | 开放实验 | [依赖注入与模拟对象](cards/40-practice.md) | I.2 |
| 41 | 开放实验 | [配置读取边界](cards/41-practice.md) | I.23, E.2 |
| 42 | 开放实验 | [错误通道契约](cards/42-practice.md) | E.2, E.6 |
| 43 | 开放实验 | [多态组合与可测试性](cards/43-practice.md) | I.2, C.120 |
| 44 | 开放实验 | [消息版本兼容](cards/44-practice.md) | I.1 |
| 45 | 开放实验 | [jthread 与 stop_token](cards/45-practice.md) | CP.25, CP.26 |
| 46 | 开放实验 | [双锁转账死锁](cards/46-practice.md) | CP.20, CP.21 |
| 47 | 开放实验 | [condition_variable 谓词](cards/47-practice.md) | CP.42 |
| 48 | 开放实验 | [原子状态机](cards/48-practice.md) | CP.1 |
| 49 | 开放实验 | [False Sharing 性能测试](cards/49-practice.md) | Per.1 |
| 50 | 开放实验 | [有界队列背压策略](cards/50-practice.md) | CP.1 |
| 51 | 开放实验 | [Future 超时与取消](cards/51-practice.md) | CP.4 |
| 52 | 开放实验 | [回调重入陷阱](cards/52-practice.md) | CP.22 |
| 53 | 开放实验 | [CMake 多库分层](cards/53-practice.md) | SF.2 |
| 54 | 开放实验 | [动态库与符号可见性](cards/54-practice.md) | I.27 |
| 55 | 开放实验 | [GDB 复现崩溃](cards/55-practice.md) | P.9 |
| 56 | 开放实验 | [Perf 热点与误优化](cards/56-practice.md) | Per.1 |
| 57 | 开放实验 | [时间戳乱序处理](cards/57-practice.md) | I.1 |
| 58 | 开放实验 | [QoS 概念映射](cards/58-practice.md) | CP.1 |
| 59 | 开放实验 | [Executor 回调组实验](cards/59-practice.md) | CP.22 |
| 60 | 开放实验 | [CLI 日志与指标](cards/60-practice.md) | I.2, Per.1 |

## 扩展任务（可选，不计入 60 题核心）

| ID | 类别 | 题目 | 规范映射 |
|---|---|---|---|
| E01 | 扩展实验 | [变参模板、SFINAE 与 concepts](extensions/E01-templates-concepts.md) | T/Con（编号以官方页面为准） |
| E02 | 扩展实验 | [STL 深水区：list、rb_tree、traits 与 EBO](extensions/E02-stl-internals.md) | SL/T（编号以官方页面为准） |
| E03 | 扩展实验 | [C++20 ranges 与 expected：传感器流水线](extensions/E03-ranges-expected.md) | SL/ES/E（编号以官方页面为准） |
| E04 | 扩展实验 | [并发内存模型：happens-before 与 memory_order](extensions/E04-memory-model.md) | CP（编号以官方页面为准） |
| E05 | 扩展实验 | [测试、CI 与包管理](extensions/E05-testing-ci.md) | SF/T（编号以官方页面为准） |

扩展任务与开放题一样，需要 `stage1/labs/<ID>/README.md` 和 `evidence.md`；用 `python3 tools/train.py show E01`、`lab E01`、`done E01 --manual`。扩展说明见 [extensions/README.md](extensions/README.md)，覆盖强度见 [Core Guidelines 覆盖矩阵](../docs/CORE_GUIDELINES_MAP.md)。

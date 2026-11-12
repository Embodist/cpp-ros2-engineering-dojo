# E04 · 并发内存模型：happens-before 与 memory_order

- **知识域**：Concurrency, Memory Model
- **类型**：扩展实验题（不提供实现和固定测试）
- **规范参考**：C++ Core Guidelines CP 系列；编号以官方页面为准

## 情境 / 任务

实现一个一次性的发布/订阅标志或序列锁：

- 用 `std::atomic` 写 relaxed、acquire/release、seq_cst 三个版本。
- 写压力测试，验证数据发布后订阅者能看到完整数据。
- 解释 relaxed 为什么可能失败，acquire/release 如何建立 happens-before。
- 跑 TSan；说明 TSan 对无锁代码能证明什么、不能证明什么。

## 可执行验收

3 个版本可切换运行；压力测试有明确迭代次数和结果；至少给出一个失败/不可复现的对照实验并解释；TSan 输出与环境记录完整。

## 提交文件

- `labs/E04/main.cpp` 或分离后的 `include/`、`src/`。
- `labs/E04/README.md`：happens-before 推导、内存序选择、何时该用 mutex。
- `labs/E04/evidence.md`：硬件/OS/编译器、迭代次数、TSan 输出、失败对照。

## 使用 AI 的边界

先自己写出三个版本和压力测试，再让 AI 追问内存序推导；不要接受“加 seq_cst 就行”这种没有解释的答案。

## ROS2 迁移问题

状态标志、回调交接、Executor 唤醒分别适合无锁原子还是 mutex？如果把 CPU 换成弱内存模型架构，哪些假设会失效？

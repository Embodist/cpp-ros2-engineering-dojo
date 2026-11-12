# E02 · STL 深水区：list、rb_tree、traits 与 EBO

- **知识域**：STL, Data Structures, Source Reading
- **类型**：扩展实验题（不提供实现和固定测试）
- **规范参考**：C++ Core Guidelines SL 系列（标准库）与 T 系列；编号以官方页面为准

## 情境 / 任务

选一个标准库实现（libstdc++ 或 libc++），完成一次源码对照：

- 跟踪 `std::list` 的节点插入/删除，画出调用链。
- 跟踪 `std::map` 背后的 rb_tree 查找路径，列出不变量。
- 用 `iterator_traits` 或自定义 trait 写一个编译期分派实验（例如区分连续/非连续迭代器）。
- 解释 EBO（空基类优化）在节点/分配器/迭代器实现中的作用。
- 用 vector/list/map 各写一个失效实验：插入或删除后，迭代器/引用/指针何时失效。

## 可执行验收

提交调用链图、不变量表、3 组失效实验的可运行测试与输出；记录源码版本/commit SHA 和文件路径；至少写一个你原先预测错误的点。

## 提交文件

- `labs/E02/main.cpp` 或分离后的 `include/`、`src/`。
- `labs/E02/README.md`：数据结构选择、复杂度、失效规则与源码结论。
- `labs/E02/evidence.md`：源码路径/commit、实验命令、输出、失败注入。

## 使用 AI 的边界

先自己画出调用链和不变量，再让 AI 指出缺口；禁止让 AI 凭记忆给行号或最新版实现。

## ROS2 迁移问题

传感器历史缓存该选 vector、deque、list 还是环形缓冲区？在热路径上，节点分配和缓存局部性会如何影响延迟？

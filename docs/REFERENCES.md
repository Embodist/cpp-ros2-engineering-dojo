# 参考资料与边界说明

资料用于**设计题目分类与学习路径**，本仓库中的文字、代码骨架和测试均独立编写，不直接复制外部仓库的大段源代码。资料分层、阅读方法和分阶段练习见 [READING_AND_PROGRESSION.md](READING_AND_PROGRESSION.md)。

1. [C++ Core Guidelines 官方](https://github.com/isocpp/CppCoreGuidelines) — 资源管理、接口、函数、类、并发和性能指导。
2. [CPlusPlusThings /《C++ 那些事》](https://github.com/Light-City/CPlusPlusThings) — 基础、实战练习、现代 C++、STL 源码、设计模式、并发。
3. [cppreference C++](https://en.cppreference.com/w/cpp) — 标准库及语言规范的实践参考。
4. [Google LevelDB](https://github.com/google/leveldb) — LSM 与 C++ 系统源码阅读材料。
5. [ROS2 Documentation](https://docs.ros.org/en/rolling/index.html) — ROS2 教程和概念文档；Rolling 会变化，实际应用与所安装发行版文档对齐。
6. [ROS2 Callback groups](https://docs.ros.org/en/rolling/How-To-Guides/Using-callback-groups.html) — 回调组与并发限制、死锁风险。
7. [ROS2 Executors](https://docs.ros.org/en/rolling/Concepts/Intermediate/About-Executors.html) — Executor 的调度模型。
8. [ament_cmake](https://docs.ros.org/en/rolling/How-To-Guides/Ament-CMake-Documentation.html) — ROS2 CMake package 基础。
9. [ISO C++ 工作草案](https://eel.is/c++draft/) — 语言规则、UB、对象生命周期和重载决议的条款级核对；引用时记录条款编号。
10. [Compiler Explorer](https://godbolt.org/) — 对比 GCC/Clang 的实现差异、代码生成和标准特性支持。
11. [WG21 提案](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/) — 特性设计动机；进阶阅读材料，不作为第一遍学习入口。
12. [ROS 2 REPs](https://ros.org/reps/rep-0000.html) — ROS 2 设计约定和接口规范。
13. [GoogleTest](https://google.github.io/googletest/) — E05 可选的生产级测试框架。
14. [Catch2](https://github.com/catchorg/Catch2) — E05 可选的轻量测试框架。
15. [Conan](https://docs.conan.io/) / [vcpkg](https://vcpkg.io/) — E05 可选依赖管理；也可用 FetchContent 固定 tag/commit。
16. [GitHub Actions](https://docs.github.com/en/actions) — E05 可选 CI；ROS2 包还需结合 colcon/ament_cmake。

**Claude Code 工具链**（详见 [CLAUDE_TOOLCHAIN.md](CLAUDE_TOOLCHAIN.md)）：

17. [Claude Code 文档](https://docs.claude.com/en/docs/claude-code) — settings、permissions、hooks、skills、subagents、plugins 的权威说明。
18. [ECC 插件](https://github.com/affaan-m/ECC) — skills/agents/hooks 的 harness 优化层（本镜像默认集成）。
19. [Anthropic 官方插件市场](https://github.com/anthropics/claude-plugins-official) — LSP 与官方集成，优先来源。
20. [Superpowers](https://github.com/obra/superpowers) — 方法论类插件；与 ECC 功能重叠，二选一。
21. [awesome-claude-code](https://github.com/hesreallyhim/awesome-claude-code) — 生态索引，用于发现，不建议整包安装。

**注意**：cppreference 是社区维护的事实参考，不是规范原文；语言争议以 ISO C++ 草案和正式标准为准。这些外部网站/源码版本随时间变化，不保证文件路径永久不变。LevelDB 跟踪具体 `Put/Get` 链路时应记录自己检出的 commit SHA；ROS 2 文档应记录发行版。E05 引入的第三方测试/依赖工具默认不影响核心 60 题的离线构建。

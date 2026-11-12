# E05 · 测试、CI 与包管理

- **知识域**：Engineering, Testing, Build
- **类型**：扩展实验题（不提供实现和固定测试）
- **规范参考**：C++ Core Guidelines SF 系列与 T 系列；编号以官方页面为准

## 情境 / 任务

选一个已完成的核心题（如 03 MiniVector 或 08 BoundedQueue），把它工程化：

- 用 GoogleTest 或 Catch2 增加测试 target；默认构建保持离线，联网依赖必须显式说明。
- 加一个 CI 工作流（GitHub Actions 或 GitLab CI）完成构建 + 测试。
- 选一种依赖管理方式（Conan、vcpkg 或 FetchContent + 固定 tag/commit），记录版本锁定。
- 保持原有自测不被破坏；写清从干净目录复现的步骤。

## 可执行验收

从全新构建目录可编译并跑通测试；CI 配置语法有效（无 CI 环境时本地模拟并记录）；依赖版本固定；README 可从新机器复现；不提交生成物。

## 提交文件

- `labs/E05/` 下的 CMakeLists、测试源文件和 CI 配置。
- `labs/E05/README.md`：构建/测试/依赖/CI 命令与取舍。
- `labs/E05/evidence.md`：干净构建日志、测试输出、依赖锁定方式、CI 结果或本地模拟结果。

## 使用 AI 的边界

先自己搭最小可运行 target，再让 AI review 构建图、依赖固定和 CI 可复现性；不要让 AI 直接生成整个仓库脚手架。

## ROS2 迁移问题

同样的测试/CI 如何迁移到 ament_cmake + colcon？ROS2 包的依赖和构建产物应该如何在 CI 中缓存与固定？

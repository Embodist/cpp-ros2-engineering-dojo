# 参考实现（从 cpp_testing 同步）

- 来源：`~/cpp_testing/reference`，git `5bbe198`（2026-11-17 同步）。
- 范围：p01–p11；p12/p13 上游尚未实现参考代码。
- 本目录只读参考；自己的实现写 `stage1/projects/pXX/`，不要直接改这里。
- 公共夹具：`stage1/projects/common/`（来自 `cpp_testing/reference/common`）。
- 项目文档：`stage1/projects/docs/`（来自 `cpp_testing/docs`）。

## 能力链

这些不是“跨仓库依赖”，而是 Stage 1 内的显式 include：

- p06 测试 → p04 List / p05 Vector
- p08 ThreadPool → p07 Function
- p11 Realtime → p06 Arena / p09 SPSC

声明在 `stage1/projects/CMakeLists.txt`。如果要求每个项目物理自包含，可以把被依赖头文件内联到项目内，但会产生重复代码。

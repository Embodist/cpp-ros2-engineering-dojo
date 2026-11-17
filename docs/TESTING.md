# 测试：离线 harness 与可选 GoogleTest

Stage 1 有两条测试路径。默认路径保持离线、零第三方依赖；GoogleTest 路径用于生产级测试训练和夹具实验。

## 1. 默认路径：自制离线 harness

```bash
python3 tools/train.py test 01
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py test 08 --sanitize tsan
python3 tools/train.py build
```

- 不联网、不依赖 GoogleTest/Catch2。
- 使用 `stage1/support/test_harness.hpp` 的 `DOJO_TEST/REQUIRE/REQUIRE_EQ/REQUIRE_THROWS`。
- `train.py done 01` 默认走这条路径。
- 适合日常训练、离线环境、快速反馈。

## 2. 可选路径：GoogleTest 镜像套件

对应 `stage1/tests_gtest/`，覆盖 01–12 和夹具自测，注册进 CTest。

```bash
# 单题（首次需要联网 FetchContent，或用 --gtest-source 指向本地源码）
python3 tools/train.py test 01 --gtest
python3 tools/train.py test 01 --gtest --gtest-source /path/to/googletest

# 构建全部 gtest target
python3 tools/train.py build --gtest --gtest-source /path/to/googletest

# 全量运行
ctest --test-dir build-gtest -L gtest --output-on-failure
```

CMake presets：

```bash
cmake --preset gtest
cmake --build --preset gtest
ctest --preset gtest -L gtest --output-on-failure

cmake --preset gtest-asan
cmake --build --preset gtest-asan
ctest --preset gtest-asan -L gtest --output-on-failure

cmake --preset gtest-tsan
cmake --build --preset gtest-tsan
ctest --preset gtest-tsan -L gtest --output-on-failure
```

### 离线构建 GoogleTest

默认使用 FetchContent 固定 `v1.15.2`。没有网络时，二选一：

```bash
cmake -S . -B build-gtest -DDOJO_USE_GTEST=ON \
  -DDOJO_GTEST_SOURCE_DIR=/path/to/googletest
# 或
cmake -S . -B build-gtest -DDOJO_USE_GTEST=ON \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=/path/to/googletest
```

`DOJO_GTEST_SOURCE_DIR` 会转成 `FETCHCONTENT_SOURCE_DIR_GOOGLETEST`，跳过下载。

## 3. 测试夹具

位置：`stage1/support/common/`，头文件按 `"common/xxx.hpp"` 引用。

| 夹具 | 用途 | 关键点 |
|---|---|---|
| `Tracer` | 统计默认/值/拷贝/移动构造、拷贝/移动赋值、析构、存活数 | 用数字证明零拷贝、走移动、构造数等于析构数 |
| `ThrowOnN` | 第 N 次构造/拷贝/移动时抛 `ThrowOnNError` | 扫描强异常保证的所有抛出点；移动标 `noexcept(false)` |
| `MoveOnly` | 不可拷贝、可 noexcept 移动 | 证明容器不依赖元素可拷贝 |
| `NoexceptToggle<B>` | 控制移动是否 noexcept | 测 `move_if_noexcept` 是搬移还是拷贝 |
| `LeakCheck` | 作用域守卫，析构时断言 Tracer 存活数回到基线 | 依赖 gtest，只在 gtest 套件中使用 |
| `NewCounter` | 替换全局 new/delete 统计分配次数/字节数 | 实现必须在 `.cpp`；nothrow new/delete 也要配对 |

夹具自身测试：`stage1/tests_gtest/fixtures_test.cpp`。

### 关于全局 new/delete

`dojo_common` 链接到所有 gtest target，会替换全局 `operator new/delete`（含 nothrow 和数组版本）。这是 `NewCounter` 的前提，但意味着：

- 只在 gtest target 中生效，不影响默认 harness；
- 计数是全局的，测量前后用 `NewCounter::reset()/allocs()/bytes()`，不要把 gtest 自身分配算进去；
- ASan 下必须同时替换 nothrow 版本，否则会出现 `alloc-dealloc-mismatch (operator new vs free)`。

## 4. CMake 选项

| 选项 | 默认 | 作用 |
|---|---|---|
| `DOJO_USE_GTEST` | OFF | 构建 GoogleTest 镜像套件 |
| `DOJO_BUILD_PROJECTS` | OFF | 构建移植的 p01–p11 参考实现/测试（要求 GoogleTest） |
| `DOJO_GTEST_SOURCE_DIR` | 空 | 本地 googletest 源码，离线构建 |
| `WARNINGS_AS_ERRORS` | OFF | 给 `dojo_warnings` 加 `-Werror` |
| `ENABLE_CLANG_TIDY` | OFF | 构建时运行 clang-tidy |
| `DOJO_SANITIZE_ADDRESS` | OFF | ASan + UBSan |
| `DOJO_SANITIZE_THREAD` | OFF | TSan（不能与 ASan 同开） |

## 5. CTest 命名与标签

- 默认测试：`dojo_smoke`、`dojo_01_scope_exit` … `dojo_12_callback_registry`，标签 `stage1`/`pending`。
- GoogleTest 测试：`gtest.dojo_gtest_01_scope_exit.ScopeExit.ExecutesOnce` …，标签 `gtest`。
- 只跑 gtest：`ctest --test-dir build-gtest -L gtest --output-on-failure`。
- 只跑某题：`ctest --test-dir build-gtest -R 'dojo_gtest_03'`。

## 6. WSL/容器下的 TSan

CMake 在 `DOJO_SANITIZE_THREAD=ON` 且 Linux 时，会给 gtest target 设置：

```cmake
TEST_LAUNCHER "setarch;${CMAKE_SYSTEM_PROCESSOR};-R"
```

这同时作用于 gtest 构建期发现和 ctest 执行。若仍遇到 `ThreadSanitizer: unexpected memory mapping`，先确认 launcher 已生效：

```bash
grep -n setarch "build-gtest-tsan/stage1/dojo_gtest_08_bounded_queue[1]_tests.cmake"
```

## 7. 新增一个 gtest 测试

1. 在 `stage1/tests_gtest/` 新建 `test_<name>_gtest.cpp`。
2. 在 `stage1/CMakeLists.txt` 的 `if(DOJO_USE_GTEST)` 块里加：
   ```cmake
   add_dojo_gtest(dojo_gtest_<id>_<name> tests_gtest/test_<name>_gtest.cpp)
   ```
3. 重新配置并运行：
   ```bash
   python3 tools/train.py test <id> --gtest --gtest-source /path/to/googletest
   ```

## 8. 已验证结果

当前环境（GCC 14，WSL2，googletest v1.15.2 本地源码）：

- 默认 harness：01–12 全部通过；01 为 7/7，03 为 5/5（含异常注入）。
- GoogleTest：79 项通过。
- GoogleTest + ASan/UBSan：79 项通过。
- GoogleTest + TSan（`setarch -R`）：79 项通过，包含 08 队列、09 线程池、12 并发回调压测。

TSan 只在 08/09/12 有实际并发路径；12 的并发测试是后加的，默认 harness 的 12 仍是单线程测试。

## 9. 项目线测试（p01–p11 参考实现）

`cpp_testing` 的 p01–p11 参考实现和测试已同步到 `stage1/projects/reference/`；p12/p13 只有需求，没有移植参考实现。

```bash
# 单项目测试（首次需 GoogleTest；可加 --gtest-source）
python3 tools/train.py project p01 --test

# 全量项目测试
ctest --test-dir build-projects -L projects --output-on-failure

# ASan / TSan
python3 tools/train.py project p11 --test --sanitize asan
python3 tools/train.py project p08 --test --sanitize tsan
```

CMake presets：`projects`、`projects-asan`、`projects-tsan`。

已验证：plain 147/147；ASan/UBSan 147/147；TSan 并发项目（p08/p09/p10/p11）30/30。

项目之间的能力链（p06→p04/p05、p08→p07、p11→p06/p09）通过显式 include 目录保留；它们都在 `stage1/projects/` 内，不再依赖外部仓库。

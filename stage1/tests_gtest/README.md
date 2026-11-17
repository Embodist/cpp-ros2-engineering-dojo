# Stage 1 GoogleTest 镜像套件

这些测试与 `stage1/tests/` 的自制 harness 版本覆盖同一组 01–12 接口，额外使用 `stage1/support/common/` 的夹具：

- `Tracer`、`ThrowOnN`、`MoveOnly`、`NoexceptToggle`、`LeakCheck`、`NewCounter`
- 03 增加异常安全扫描、Tracer 平衡、`move_if_noexcept` 分支验证
- 08/09/12 增加并发路径；12 的并发 subscribe/publish/unsubscribe 让 TSan 有意义

启用方式：

```bash
python3 tools/train.py test 01 --gtest
ctest --test-dir build-gtest -L gtest --output-on-failure
```

GoogleTest 通过 FetchContent 固定 `v1.15.2`；离线时用 `--gtest-source` 或 `-DDOJO_GTEST_SOURCE_DIR=...`。

详细说明见 [docs/TESTING.md](../../docs/TESTING.md)。

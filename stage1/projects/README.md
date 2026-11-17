# Stage 1 项目工作区

项目线把 `stage1/projects` 的 p01–p13 接到 Stage 1 的先修题上。完整映射见 [../PROJECT_TRACK.md](../PROJECT_TRACK.md)。

## 代码在哪

- **项目代码**：`stage1/projects/pXX/`（自己的仓库，自己写）。
- **参考实现**：`stage1/projects/reference/pXX/`（只看，不照抄）。
- **本目录**：每个项目的项目卡/检查表，例如 `stage1/projects/p01/README.md`、`CHECKLIST.md`。
- **证据**：`notes/projects/pXX.md`。

参考实现已同步到 `reference/`；工作区 `pXX/` 用于自己写实现，不要直接改参考实现。

## CLI

```bash
# 列出项目线（含状态、先修题缺口、最小闭环进度）
python3 tools/train.py projects

# 只看最小闭环
python3 tools/train.py projects --minimal

# 查看某个项目
python3 tools/train.py project p01

# 生成项目卡/检查表/证据模板，并标记 WIP
python3 tools/train.py project p01 --start

# 证据写完且没有 TODO 后标记完成
python3 tools/train.py project p01 --done
```

`project --start` 会创建：

```text
stage1/projects/p01/README.md      # 项目卡：先修题、里程碑、能力承接/输出
stage1/projects/p01/CHECKLIST.md   # 先修、测试、sanitizer、证据、复盘
notes/projects/p01.md              # 证据模板
```

## 最小闭环

```text
p01 v1→v2/v3 → p02a → p05 → p08 → p09 → p10 → p11 → p12 → p13
```

可选补齐：p02b、p03、p04、p06、p07。p06 对 p11 是推荐，不是硬依赖；p12 是 p13 的硬前置。

## 能力连续

每个项目卡都有“能力承接 / 能力输出”：

- 承接：上一阶段已经练过的动作。
- 输出：下一个项目/Stage 2 会直接复用的能力。

`projects` 命令会按先修题和项目依赖提示下一步；不要跳过硬前置（例如 p13 前先完成 p10/p11/p12）。

## 构建与测试移植后的参考实现

```bash
python3 tools/train.py project p01 --test
python3 tools/train.py project p08 --test --sanitize tsan
ctest --test-dir build-projects -L projects --output-on-failure
```

或使用 presets：`projects`、`projects-asan`、`projects-tsan`。

参考测试在 `stage1/projects/reference/pXX/tests`；自己的测试写 `stage1/projects/pXX/tests`。p12/p13 暂无移植参考实现。

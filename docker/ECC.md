# ECC 插件（Claude Code）使用说明

> 镜像默认**预装并启用** [ECC](https://github.com/affaan-m/ECC)，进入容器 `claude` 即可用，无需手动安装。
> 本文解释它装了什么、有什么代价、放在哪、怎么验证和调整——避免“黑盒”。

## 1. 它是什么

ECC 是一个面向 Claude Code 的「harness 优化层」，当前版本 **2.2.3**，包含：

| 组件 | 数量 | 说明 |
| :--- | :--- | :--- |
| Skills | 427 | 按需调用的技能（`/skill` 或自动触发） |
| Agents | 72 | 子代理（code-reviewer、cpp-reviewer、tdd-guide…） |
| Commands | ~99 | 旧的命令 shim（逐步被 skills 取代） |
| Hooks | 8 类生命周期事件 | PreToolUse / PostToolUse / UserPromptSubmit / SessionStart / PreCompact / PostToolUseFailure / Stop / SessionEnd |
| MCP server | 1 | chrome-devtools |

Marketplace 名和插件名都叫 `ecc`，所以安装标识是 `ecc@ecc`。

## 2. 代价（先看这个）

- **常驻上下文**：全量启用时，每个会话大约 **+35k tokens** 的 always-on 上下文（列表模式下）。对教学/多人共用的镜像，这是实打实的成本与延迟。
- **Hooks 默认开启**：`hooks_enabled=true`、`hook_profile=standard`，会在每次工具调用/会话生命周期触发本地钩子（安全、质量、格式化等自动化）。这是“开箱即用”的一部分，但确实会改变默认行为。
- **磁盘**：未裁剪约 **+390MB**（marketplace 克隆约 108MB + 插件 cache 约 285MB，其中 `node_modules` 约 211MB）。构建参数 `ECC_PRUNE_DEPS=1`（默认）会用 `npm prune --omit=dev` 裁掉 devDependencies，显著变小。

> 只想“轻量用一部分”时，可改用精简安装：
> `npx ecc-universal@2.2.3 install --profile minimal --target claude --yes`

## 3. 安装位置（镜像里怎么装的）

Dockerfile 里以 `dev` 用户（vscode 变体是 `user`）执行：

```bash
claude plugin marketplace add https://github.com/affaan-m/ECC
claude plugin install ecc@ecc --scope user
npm prune --omit=dev   # 仅当 ECC_PRUNE_DEPS=1
```

落到以下位置：

| 路径 | 作用 |
| :--- | :--- |
| `~/.claude/plugins/marketplaces/ecc/` | marketplace 克隆（更新用） |
| `~/.claude/plugins/cache/ecc/ecc/<version>/` | 实际加载的插件（含 `node_modules`） |
| `~/.claude/plugins/installed_plugins.json` | 记录安装 scope / version / git commit |
| `~/.claude/plugins/known_marketplaces.json` | marketplace 来源 |
| `~/.claude/settings.json` | `enabledPlugins`（安装时自动 enable） |

主镜像里 `~` 是 `/home/dev`；compose 用命名卷 `claude-config:/home/dev/.claude` 持久化。

> ⚠️ **命名卷注意**：Docker 只在卷为空时把镜像内 `.claude` 复制进去。**老用户已有的 `claude-config` 卷不会自动获得新镜像里的 ECC**；需要 `docker volume rm cpp-ros2-dojo_claude-config` 重建卷，或进容器手动执行上面的安装命令。新学生/新卷则为开箱即用。

## 4. 验证（构建后 / 进容器）

```bash
# 1) 插件是否已注册且启用
claude plugin list            # 应看到 ecc@ecc  Status: enabled

# 2) marketplace / cache 是否就位
ls ~/.claude/plugins/marketplaces/ecc >/dev/null && echo marketplace-ok
ls -d ~/.claude/plugins/cache/ecc/ecc/*/ >/dev/null && echo cache-ok

# 3) 完整自检（仓库根目录，容器内）
./docker/dev.sh bash docker/smoke.sh      # 看 "Claude Code / ECC" 段
```

`docker/smoke.sh` 的 ECC 段会检查：marketplace 已克隆、cache 存在、`ecc@ecc` 已安装且 enabled。

## 5. 调整与关闭

进入容器后：

```bash
# 查看 ECC 配置项（hooks_enabled / hook_profile）
/plugin            # 交互式：选中 ecc@ecc → configure

# 命令行非交互设置（可在 Dockerfile/CI 里用）
claude plugin install ecc@ecc --scope user \
  --config hooks_enabled=false \
  --config hook_profile=minimal      # minimal | standard | strict

# 临时禁用（保留文件）
claude plugin disable ecc@ecc
claude plugin enable  ecc@ecc

# 彻底卸载
claude plugin uninstall ecc@ecc
claude plugin marketplace remove ecc
```

构建期控制：

```bash
INSTALL_ECC=0 ./docker/build.sh          # 不预装 ECC
ECC_PRUNE_DEPS=0 ./docker/build.sh       # 保留全部 node_modules（体积更大，功能最全）
```

## 6. 和 C++/ROS2 训练营相关的常用能力

无需记全 400+ 个 skill，先掌握这些即可：

- **构建/修复**：`cpp-build`、`build-fix`、`cpp-build-resolver`（agent）、`cmake` 相关
- **代码质量**：`cpp-review`、`cpp-reviewer`（agent）、`code-review`、`cpp-coding-standards`
- **测试驱动**：`cpp-test`、`cpp-testing`、`tdd-workflow`、`tdd-guide`（agent）
- **验证/收尾**：`verification-loop`、`quality-gate`、`refactor-clean`
- **通用工程**：`git-workflow`、`conventional-commit-review`、`codebase-onboarding`、`plan`

示例：

```bash
claude "用 cpp-review 审一遍 stage1/scope_exit 的实现"
claude "按 tdd-workflow 给 stage1 的 task 03 补一组测试"
claude "用 build-fix 处理 stage2/ros2_ws 的 colcon 编译错误"
```

## 7. 可复现性

`installed_plugins.json` 会记录安装时的 `gitCommitSha`。marketplace 指向上游 `main`，**不是**锁死版本；如需可复现构建，可在 CI 里记录该 commit，或把 ECC 克隆到固定 commit 后用路径方式 `marketplace add`。

## 8. 安全

- 镜像**不固化任何 Claude 鉴权**；`ANTHROPIC_AUTH_TOKEN` 等仍走运行时环境变量。
- ECC 的 hooks 默认开启，会执行本地脚本；如教学环境要求“零自动化”，用 `--config hooks_enabled=false` 或 `INSTALL_ECC=0`。
- 插件代码来自第三方仓库，启用前请自行评估；本镜像只做预装 convenience，不代表背书。

## 9. 插件选型与优化（避免叠装）

2026 年的社区共识（多来源）：

- **不是装得越多越好**：每个插件/skill 都占上下文与 token。
- **Superpowers 与 ECC 功能高度重叠**（都覆盖 TDD、brainstorm→plan→implement、code review、systematic-debugging）。
  - 本镜像默认**同时预装了 ECC 与官方 `superpowers`、`clangd-lsp`**（按需求，可验证可用），常驻上下文/token 成本更高。
  - 想精简：用构建参数 `CLAUDE_EXTRA_PLUGINS` 只保留其一，或 `INSTALL_CLAUDE_OFFICIAL=0` 只留 ECC。
- **LSP 优先、MCP 克制**：LSP 的补全/诊断 token 成本远低于常驻 MCP。C++/ROS2 真正需要的是 `clangd-lsp`；`typescript-lsp`、`rust-analyzer-lsp` 对本训练营基本无用，可从个人配置移除。
- **团队权限模板**：仓库已提交 `.claude/settings.json`（allow 构建/测试/只读 git，deny 密钥与危险命令）；个人覆盖写 `.claude/settings.local.json`（已 gitignore）。
- **全新会话验证**：`/context`、`/permissions`、`/hooks`、`/doctor` 反映会话启动时加载的配置，改完请开新会话确认。

> 参考：Anthropic 官方插件市场（`anthropics/claude-plugins-official`）、`wshobson/agents`（按栈装 2–3 个）、`obra/superpowers`、`awesome-claude-code`。以上为社区资料，版本敏感，以 `claude --version` 与官方文档为准。

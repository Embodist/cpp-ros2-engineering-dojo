# 工业级 Claude Code 最佳实践（给真实代码开发）

> 目标：把 Claude Code 调成**可预测、可验证、可复现、最小权限**的工程伙伴。
> 面向 C++/ROS2，但原则适用于任何工业级仓库。配置示例见本仓库 `.claude/settings.json`。

## 1. 五条铁律

1. **验证优先**：先给 Claude 一个“能自证”的手段（测试/构建/CI/独立复核），再谈生成。这比任何 prompt 都有效。
2. **常驻最小**：每个 always-on 的 skill/agent/MCP 都烧 token。能按需就别常驻。
3. **每轴一个**：语言智能、代码审查、方法论、安全——**每个维度只留一个**，避免“该听谁的”。
4. **确定性护栏**：能用 `permissions.deny` 拦的，别靠提示词；能用 hook 强制的，别靠自律。
5. **可复现 + 最小权限**：锁版本、可回滚、密钥不入库、危险操作需人确认。

## 2. 插件分层（按轴，装全但按需开）

| 轴 | 选型 | 备注 |
| :--- | :--- | :--- |
| 语言智能 | `clangd-lsp`（+`lua-lsp`/`pyright-lsp` 视栈） | LSP 常驻成本远低于 MCP，优先 |
| 代码审查 | `pr-review-toolkit` **或** `code-review` | 二选一；另不要与 ECC/superpowers 的 review 叠加 |
| 方法论 | `superpowers` **或** `ecc@ecc` **或** `feature-dev` | **只留一个**（工业推荐 superpowers：聚焦、可预测） |
| 安全 | `security-guidance` **或** ECC 安全 | 一个即可 |
| Git/提交 | `commit-commands` | 配合权限 `ask` 把关 push |
| 元工具 | `claude-code-setup`、`skill-creator`、`claude-md-management`、`explanatory-output-style` | 偶用，默认关，用时再开 |

**本仓库默认启用（7）**：`clangd-lsp`、`lua-lsp`、`pyright-lsp`、`commit-commands`、`security-guidance`、`pr-review-toolkit`、`superpowers`。
**装好但默认关（8）**：`ecc@ecc`、`code-review`、`code-simplifier`、`feature-dev`、`claude-code-setup`、`claude-md-management`、`skill-creator`、`explanatory-output-style`。

> 想在项目里改用 ECC 作方法论：把 `ecc@ecc` 设 `true`、`superpowers` 设 `false`（二选一）。插件已装在镜像里，切换只是开关。

## 3. 权限矩阵（三档）

写在项目 `.claude/settings.json`（团队标准，入库）：

- **allow（放行高频安全操作）**：只读 git（status/diff/log/show/branch/remote）、`rg/fd`、构建测试（`cmake/ctest/colcon/ninja/make/g++/clang*/ccache`）、`python3 tools/train.py`。
- **ask（人来把关）**：`git push`、`git merge`、`git rebase`、`git reset` —— 保持**提交历史由人决定**。
- **deny（硬拦截）**：读密钥（`.env`、`auth.json`、`*.pem`、`*.key`、`~/.ssh`、`~/.aws`）；破坏性（`rm -rf`、`sudo`、`git push --force`、`mkfs`、`dd`、`chmod -R 777`）。

个人偏好写 `.claude/settings.local.json`（已 gitignore）；安全硬约束应放 managed/组织层。

## 4. 确定性护栏：deny + hooks

- **硬拦截放 `permissions.deny`**：这是确定性的，Claude 无法绕过（上面已配置）。
- **自动化放 hooks**（要求“每次都发生”的事），但**同类只留一套**，否则每次工具调用被拦截多次。示例（`settings.json` 的 `hooks`）：

```jsonc
{
  "hooks": {
    "PostToolUse": [
      { "matcher": "Edit|Write",
        "hooks": [ { "type": "command", "command": "$CLAUDE_PROJECT_DIR/.claude/hooks/format-cpp.sh" } ] }
    ],
    "Stop": [
      { "hooks": [ { "type": "command", "command": "ctest --test-dir build -R '^dojo_smoke$' --output-on-failure" } ] }
    ]
  }
}
```

- 落地前先在**一次性仓库**试；hook 脚本要幂等、快速、失败可解释。`ECC` 自带 hooks，启用 ECC 时**不要再叠**同类全局 hooks。

## 5. 上下文与成本

- 噪声大/批量/独立的活 → 丢给 **subagent**（独立上下文，只回结论）。
- 无关任务间 `/clear`，长任务 `/compact`；大改动先 **plan**（Shift+Tab）再实现。
- 并行：多用 **git worktree**（多个会话并行，最大提效项）。
- **LSP 优先、MCP 克制**：MCP 的 tool schema 常驻且会增加误选工具。

## 6. CLAUDE.md / rules / commands / agents 分工

- `CLAUDE.md`：<~200 行，只写“代码里看不出、删了会犯错”的东西（非显然的构建/测试命令、约定、边界、坑）。
- `.claude/rules/*.md`：带 `paths:` frontmatter 的**路径限定**硬规则，只在相关文件加载。
- `.claude/commands/*.md`：人手动触发的固定流程；`.claude/agents/*.md`：独立上下文的角色代理。

## 7. 验证门槛（工业核心）

- 每个改动都要有**可执行验证**：`ctest` / `colcon build --symlink-install` / sanitizer / 单元测试。
- 用 **Stop hook** 或 **verifier 子代理** 作为“未过检查不许收工”的闸门。
- CI 作为最终裁判；本地用 `./docker/dev.sh bash docker/smoke.sh` 做环境自检。

## 8. 可复现与供应链

- 插件/LSP **锁版本或 commit**；镜像构建参数固定（Node/nvim/Claude/ECC 版本可覆盖）。
- 第三方插件会带文件/脚本/MCP/运行行为：先 review，可回滚，官方市场优先。
- 镜像用 `dojo:latest` + `:vscode`（FROM base）等**可复现标签**；生产用 `sha-<commit>`。

## 9. 安全

- 密钥/凭据不进仓库、不进镜像、不进 `settings.json`；走运行时环境变量。
- `permissions.deny` 拦密钥读取；`bypassPermissions` **只在无外网的隔离沙箱**。
- 访问令牌最小权限 + 定期轮换；不在对话/日志里长期暴露。

## 10. 落地清单（新仓库/新项目开箱）

- [ ] 提交 `.claude/settings.json`：allow/ask/deny 三层 + 分层 `enabledPlugins`。
- [ ] `CLAUDE.md` 精简；必要时加 `.claude/rules/`。
- [ ] 选定**每轴一个**插件（方法论 = superpowers 或 ecc，二选一）。
- [ ] 加至少一条验证路径（测试/构建/CI），并设为 Stop hook 或 verifier。
- [ ] 锁版本 + 可回滚；deny 密钥与危险命令。
- [ ] 新会话用 `/context`、`/permissions`、`/hooks`、`/doctor` 复核。

## 11. 本仓库默认摘要

- 方法论：**superpowers**（ECC 装好但默认关）。
- 审查：`pr-review-toolkit`（`code-review` 关）。
- 安全：`security-guidance`。
- 语言：`clangd-lsp` + `lua-lsp` + `pyright-lsp`。
- 权限：构建/测试/只读 git 放行；push/merge/rebase/reset 需确认；密钥与破坏性命令拒绝。

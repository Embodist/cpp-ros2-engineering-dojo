# Claude Code 工具链与最佳实践（Dojo）

> 目标：让每个学生和维护者都能**理解并掌控**容器里的 Claude Code，而不是黑盒。
> Claude Code 迭代很快（本文对照 2026 年的 2.1.x 实践），以 `claude --version`、`/help` 和官方文档为准。
> ECC 插件专门说明见 [../docker/ECC.md](../docker/ECC.md)。
> 面向工业级代码开发的落地实践见 [INDUSTRIAL_BEST_PRACTICES.md](INDUSTRIAL_BEST_PRACTICES.md)。

## 1. 镜像里的 Claude Code 组成

| 组件 | 位置 / 版本 | 说明 |
| :--- | :--- | :--- |
| Claude Code CLI | 全局 npm，`CLAUDE_CODE_VERSION`（见 Dockerfile） | `claude` 命令 |
| ECC 插件 | `~/.claude/plugins/`（`ecc@ecc`，默认启用） | skills/agents/hooks，见 ECC.md |
| C/C++ LSP | `clangd`（apt） | 容器内由 nvim/编辑器复用；Claude 侧可加 `clangd-lsp` |
| MCP servers | **Claude 默认无**；Pi 有 filesystem/mini/searxng | 是否给 Claude 加见第 8 节 |
| 主配置 | `~/.claude.json`（最小） | 不含密钥 |
| 用户设置 | `~/.claude/settings.json` | 由插件安装/`/config` 生成 |

> 密钥（`ANTHROPIC_AUTH_TOKEN` 等）永远走运行时环境变量，不进镜像。

## 2. 配置分层与优先级

Claude Code 配置有作用域，优先级大致为：

```
managed/org > CLI flags > .claude/settings.local.json > .claude/settings.json(项目) > ~/.claude/settings.json(用户)
```

- **数组型配置**（如 `permissions.deny`）跨层**合并**，不是覆盖——团队 deny 不会被个人文件悄悄放开。
- 本仓库已提交 `.claude/settings.json`（团队共享）；个人覆盖写 `.claude/settings.local.json`（已被 `.gitignore` 忽略）。
- 安全相关的硬性限制应放 managed/团队层；个人文件只放自己的偏好。

| 文件 | 归属 | 是否入库 |
| :--- | :--- | :--- |
| `.claude/settings.json` | 团队约定（权限模板） | ✅ 提交 |
| `.claude/settings.local.json` | 个人覆盖 | ❌ 忽略 |
| `~/.claude/settings.json` | 用户全局 | 镜像内生成 |
| `CLAUDE.md` | 项目记忆 | ✅ 提交 |

## 3. 六种扩展机制怎么选（速查）

| 你想… | 用 |
| :--- | :--- |
| 让 Claude **知道**某事实/约定 | `CLAUDE.md` |
| 某件事**必须每次发生**（格式化、拦截危险命令） | **hooks** |
| 打包一段**可复用流程/领域知识** | **skills** |
| 做会**污染主上下文**的调研/批处理/独立验证 | **subagents** |
| 开发者**手动触发**的固定流程 | **commands** |
| 按**文件路径**生效的硬约束 | **rules**（`.claude/rules/*.md` + `paths:` frontmatter） |

一句话：commands 是“人让 Claude 做的”，skills 是“Claude 知道的”，rules 是“Claude 必须守的”，hooks 是“Claude 绕不过去的”。

## 4. CLAUDE.md 写法

- **保持精简**（社区经验：< ~200 行，超了模型遵从度会下降）。每行自检：“删掉它会不会让 Claude 出错？”不会就删。
- 只写**代码里看不出来**的东西：非显然的构建/测试命令、与语言默认不同的约定、架构边界、分支/PR 规则、环境坑。
- **不写**：目录清单、框架版本、通用风格指南、长 API 文档（这些要么能推导，要么会腐烂）。
- 作用域：managed → `~/.claude/CLAUDE.md` → `./CLAUDE.md` → `CLAUDE.local.md`（忽略）；子目录 CLAUDE.md 在读取该目录文件时惰性加载。
- 用 `@path/to/file` 引入；大段规则拆到 `.claude/rules/` 并用 `paths:` 限定时机，让规则只在相关文件上加载。

本仓库的 `CLAUDE.md` 只放了训练命令、C++20/格式化约定、`ctest` 初始失败属预期、以及“别改判分逻辑骗测试”。

## 5. 权限（Permissions）

- 三个动作：`allow` / `ask` / `deny`。
- 已提交的 `.claude/settings.json` 模板：
  - **allow**：`git status/diff/log/show/branch`、`rg`、`fd`、`cmake`、`ctest`、`colcon`、`ros2`、`ninja`、`make`、`g++`、`clang*`、`python3 tools/train.py`
  - **deny**：`Read(./.env)`、`Read(./docker/.env)`、`Read(**/auth.json)`、`Read(**/*.pem|*.key)`、`Bash(rm -rf:*)`、`Bash(sudo:*)`、`Bash(git push --force:*)`
- 用 `/permissions` 管理；跑几轮后可用 `/fewer-permission-prompts` 收敛常见命令。
- **不要**为了省事提交过宽的 allow，更不要在共享环境用 `bypassPermissions`（只应在无外网的隔离沙箱里）。

## 6. Hooks（确定性自动化）

- Hooks 是 `settings.json` 里 `hooks` 键的**生命周期处理器**（PreToolUse / PostToolUse / UserPromptSubmit / SessionStart / Stop / SessionEnd 等），用来做“必须每次发生、模型无法绕过”的事。
- **ECC 默认开启 hooks**（`hooks_enabled=true`、`hook_profile=standard`，见 ECC.md）。教学环境想“零自动化”：
  ```bash
  claude plugin install ecc@ecc --scope user --config hooks_enabled=false
  ```
- 自建 hook 示例（写进项目 `.claude/settings.json` 的 `hooks`）：
  - `PostToolUse`（匹配 Edit/Write）→ 对改动文件跑 `clang-format -i`（C++ 场合适用）。
  - `PreToolUse`（匹配 Bash）→ 拦截 `rm -rf`/`sudo` 等危险命令。
  - `Stop` → 阻断“未过测试就宣布完成”，强制跑一次 `ctest -R dojo_smoke`。
- 改完用 `/hooks` 在**新会话**确认（会话启动时才加载）。

## 7. 插件（Plugins）

- 本镜像默认集成 **ECC**（广度型“操作系统层”）。成本：约 +35k 常驻 tokens、hooks 默认开、约 +390MB（默认已裁 devDeps）。
- **不要叠装重叠框架**：`Superpowers` 与 ECC 都覆盖 TDD/plan/review/debug，**二选一全量**。
- 其他来源按需、少量：
  - 官方市场 `anthropics/claude-plugins-official`（LSP、GitHub/Sentry/Vercel 集成等，最高信号）。
  - `wshobson/agents`（按技术栈只装 2–3 个，不要整包）。
  - `awesome-claude-code` 用于**发现**，不建议整包安装。
- 选型原则：**LSP 优先、MCP 克制**（LSP 补全/诊断的常驻成本远低于 MCP 的 tool schema）。
- 供应链：第三方插件可带文件/脚本/MCP/运行行为；装前 review，先在不重要的仓库试，保持**可逆**。

```bash
claude plugin list                         # 已装/启用状态
claude plugin details ecc@ecc              # 组件清单 + 预估 token
claude plugin disable ecc@ecc              # 临时禁用
claude plugin install <plugin>@<market>    # 安装
claude plugin update <plugin>              # 更新（需重启生效）
```

## 8. MCP（给 Claude）

- 镜像里 Pi 自带 filesystem / mini / searxng（`docker/mcp.json`），**那是 Pi 的配置，Claude 不会自动读**。
- 给 Claude 加 MCP 的常见方式：
  - 用户级：`claude mcp add ...`，写入 `~/.claude.json` 的 `mcpServers`。
  - 项目级：仓库根 `.mcp.json`（团队共享）。
- 注意：MCP 的 tool 名/schema 常驻上下文，且服务器失败会拖慢会话；能用原生工具/LSP 就不要加 MCP。

## 9. Subagents / Skills / Commands / Rules 目录

```
.claude/
├── settings.json        # 团队权限/钩子（已提交）
├── settings.local.json  # 个人覆盖（忽略）
├── commands/*.md        # 手动触发的斜杠命令
├── skills/<name>/SKILL.md   # 按需加载的领域流程
├── rules/*.md           # 路径限定的硬规则（frontmatter: paths:）
└── agents/*.md          # 独立上下文的子代理
```

- ECC 已提供大量 skills/agents/commands；项目级只需补**本仓库特有**的少量（如“给某任务补测试”的固定流程）。
- 子代理适合：跑测试、批量审计、独立复核——只把结论带回主上下文，避免噪声。

## 10. 工作流最佳实践

**第一优先级：让 Claude 有办法验证自己的工作**（测试、构建、截图、Stop hook、独立 verifier 子代理）。这比任何 prompt 技巧都有效。

- 超过一句话的改动先 **plan**（Shift+Tab），大改动先出 spec 再在干净会话实现。
- **上下文管理**：无关任务之间 `/clear`；长任务 `/compact`；噪声工作丢给 subagent。
- **并行**：多个 git worktree / 会话并行（被广泛认为是最大提效项）；容器内可用 `git worktree`。
- **把设置共享给团队**：提交 `.claude/settings.json`、rules、commands，让所有人行为一致。

| 维度 | 黑客松（求快） | 工业级（求稳） |
| :--- | :--- | :--- |
| 扩展 | 一个强插件 + 快速脚手架 | 精选少量 + 锁定版本 + 可复现 |
| 验证 | 能跑起来即可 | 测试/构建/CI/独立复核为门槛 |
| 权限 | 宽松 allow，快速迭代 | 三级权限 + deny 密钥/危险命令 |
| 配置 | 个人本地文件 | 团队 `settings.json` 入库，managed 管控安全项 |
| 上下文 | 能省则省 | 子代理隔离 + 定期清理 + 可观测 |
| 安全 | 事后再说 | 供应链 review、密钥不进仓、最小权限 |

## 11. C++ / ROS2 专用

- **clangd + `compile_commands.json`**：容器以宿主机同绝对路径挂载工程，先 `python3 tools/train.py build` 生成数据库，clangd 才能正确补全/跳转。
- **验证手段就是你的护栏**：
  - Stage 1：`python3 tools/train.py test NN`、`ctest --test-dir build -R '^dojo_NN_' --output-on-failure`
  - Sanitizer：`python3 tools/train.py test 03 --sanitize asan|tsan`
  - ROS 2：`colcon build --symlink-install --packages-select robot_dojo_pipeline`
- ECC 里与本训练营相关的技能：`cpp-build` / `build-fix`、`cpp-review`、`cpp-coding-standards`、`cpp-test`/`tdd-workflow`、`verification-loop`、`code-review`、`git-workflow`。

```bash
claude "用 cpp-review 审 stage1/scope_exit 的异常安全"
claude "按 tdd-workflow 给 task 03 补边界测试，并跑 asan"
claude "用 build-fix 处理 stage2/ros2_ws 的 colcon 报错"
```

## 12. 验证与排障

**改完配置务必开新会话**验证（`/context`、`/permissions`、`/hooks`、`/doctor` 反映会话启动时加载的配置）。

| 现象 | 排查 |
| :--- | :--- |
| 进容器看不到 ECC | 用的旧命名卷 `claude-config`；`docker volume rm cpp-ros2-dojo_claude-config` 重建，或手动装 |
| 插件没生效 | `claude plugin list` 看是否 enabled；`claude plugin enable ecc@ecc` |
| hooks 总是拦截/变慢 | `--config hooks_enabled=false` 或 `/plugin configure` 降低 profile |
| MCP server 连不上 | 检查路径/命令；searxng 需 `SEARXNG_URL`；先 `claude mcp list` |
| 鉴权失败 | 设 `ANTHROPIC_AUTH_TOKEN` + `ANTHROPIC_BASE_URL`，或容器内 `claude` 按提示登录 |
| 改了 settings 没反应 | 在旧会话里改的；开新会话 |
| 构建自检 | `./docker/dev.sh bash docker/smoke.sh`（含 ECC 段） |

## 13. 安全

- 密钥/凭据不进镜像、不进仓库：deny `Read(.env / auth.json / *.pem / *.key)`；`docker/.env` 已忽略。
- **GitHub token 用最小权限**并定期轮换；不要在对话/日志里明文长期暴露。
- 第三方插件/技能属于供应链：先 review、可回滚、不确定先在一次性仓库试。
- `bypassPermissions` 只在无外网的隔离沙箱；共享环境坚持 allowlist/auto。

## 14. 新增工具/插件 Checklist

- [ ] 真的需要吗？能否用已有 skill/原生工具/LSP 替代？
- [ ] 成本：常驻 tokens、磁盘、hooks 行为是否可接受？
- [ ] 来源可信？版本是否可锁定（commit/tag）？
- [ ] 是否与已有插件重叠（尤其 ECC vs Superpowers）？
- [ ] 配置写在哪一层（团队 vs 个人），是否会被误提交密钥？
- [ ] 新会话验证 `/context`、`/permissions`、`/hooks`、`/doctor`；跑 `docker/smoke.sh`。

## 15. 参考

- Anthropic 官方插件市场：`anthropics/claude-plugins-official`
- ECC：<https://github.com/affaan-m/ECC>
- Superpowers：<https://github.com/obra/superpowers>
- wshobson/agents：<https://github.com/wshobson/agents>
- awesome-claude-code（发现用）：<https://github.com/hesreallyhim/awesome-claude-code>
- 以上为社区资料，版本敏感；以 `claude --version` 与官方文档为准。

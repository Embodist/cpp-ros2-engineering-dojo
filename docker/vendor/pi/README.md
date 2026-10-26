# dotfiles.pi

Pi 编码代理 + Claude Code 的**可移植配置快照**与配套工具。拷走整个项目目录,换机几分钟就能跑通。

## 目录结构

| 路径 | 内容 |
|------|------|
| `config/` | 运行时配置快照:`settings.json` / `models.json`(deepseek-v4-flash 1M 上下文,800K 压缩阈值)/ `extensions/`(plan-mode, subagent, sensitive-guard)/ `agents/` `prompts/` `skills/`。**全实体文件,无 symlink** |
| `.mcp.json` | MCP 配置:`filesystem` + `mini` + `searxng`(本地 SearXNG,相对路径) |
| `mcp-servers/` | 本地零依赖 MCP server:`mini-server.js`、`searxng-server.js` |
| `tools/` | 长程自动循环:`pi-autorun.js`(pi RPC)、`claude-autorun.mjs`(Claude Agent SDK) |
| `PI_*.md` | 安装 / 能力 / 用法文档(中文) |

## 快速开始(新机)

1. 拷走整个项目目录。
2. 装 pi(国内先换源):
   ```bash
   npm config set registry https://registry.npmmirror.com
   npm install -g --ignore-scripts @earendil-works/pi-coding-agent
   ```
3. 应用配置快照:
   ```bash
   mkdir -p ~/.pi/agent
   cp -rL config/settings.json config/models.json config/extensions config/agents config/prompts config/skills ~/.pi/agent/
   ```
4. 填密钥(仓库里只有占位符):
   ```bash
   cp config/auth.json ~/.pi/agent/auth.json && vi ~/.pi/agent/auth.json   # key 改真实值
   chmod 600 ~/.pi/agent/auth.json
   ```
5. 修 `.mcp.json` 机器相关路径:`filesystem` 的 `npx` 用本机 node 路径(普通 Linux 用 `"npx"`);`searxng` 的 `SEARXNG_URL` 按新机地址改。

## 长程自动循环

```bash
# pi 后端(N 轮循环,[TASK_COMPLETE] 单独成行即提前结束)
node tools/pi-autorun.js "任务描述" 5 --out run.log

# Claude Code 后端(SDK 驱动,跨轮续同一会话)
cd tools && npm install          # 只需一次,装 @anthropic-ai/claude-agent-sdk
node tools/claude-autorun.mjs "任务描述" 5 --out run.log
```

env 可覆盖权限 / 工具白名单 / 模型等(见各脚本头部注释):
- `CLAUDE_PERMISSION=bypassPermissions`(yolo,默认)| `acceptEdits` | `default` | `plan` | `dontAsk`
- `CLAUDE_ALLOWED_TOOLS="Read,Grep,Glob,Bash"` / `CLAUDE_DISALLOWED_TOOLS="Bash:rm -rf"`

## 安全

- 真实密钥只在 `~/.pi/agent/auth.json`(0600);仓库里只有占位符模板 `config/auth.json`。
- `.claude/`(曾含历史真实 key)已 gitignore,不入库。
- 默认 YOLO 自动执行,仅拦敏感路径;清单可在 `config/extensions/sensitive-guard.ts` 顶部改。

## 文档

- `PI_AGENT_INSTALL.md` — 面向 AI agent 的安装速查(含关键坑位 P1–P11)
- `PI_INSTALL.md` — 从零安装 pi 完整步骤(Aliyun Node 源)
- `PI_INSTALL_PROMPT.md` — 一键安装 prompt(原样复制给 Claude Code / Codex 等 AI agent 自动执行)
- `PI_USAGE.md` — 使用文档:接入 Mera AI 自定义 provider 与扩展
- `PI_CAPABILITY.md` — 本机能力清单与复验命令
- `PI_AGENT_CALL.md` — 跨 Agent 调用:调用 Claude Code 的三种方式 + 两边安装/协商步骤(实测核实)
- `SEARCH_AGENT_DESIGN.md` — 搜索 Agent 设计稿:知识库分布、向量库、工具暴露、rerank(待构思落地)
- `PI_GUIDE.md` — MCP / Skills / 自主能力进阶

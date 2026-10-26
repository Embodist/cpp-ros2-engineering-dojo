# Pi 本机能力清单(环境快照)

> 记录 2026-08-26 当前这台机器(WSL2)Pi 环境的**已装能力**与**复验命令**。
> AI agent 装/验之前先读这个,按"目标状态"收敛。配套:`PI_AGENT_INSTALL.md`(安装速查)、`PI_INSTALL.md`(详细)、`PI_AGENT_CALL.md`(跨 Agent 调用)、`PI_GUIDE.md`(进阶)。

---

## 1. 环境速览

| 项 | 值 |
|----|-----|
| OS | Linux 6.6.114.1-microsoft-standard-WSL2 |
| Node | v22.22.2(`/usr/local/node-v22.22.2-linux-x64-glibc-217/`) |
| npm | 10.9.7 |
| pi | 0.84.3(`@earendil-works/pi-coding-agent`) |
| npm registry | `https://registry.npmmirror.com`(Aliyun,`~/.npmrc`) |
| pi 可执行 | `/usr/bin/pi` → `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/pi` |
| npm global root | `/usr/local/node-v22.22.2-linux-x64-glibc-217/lib/node_modules` |
| **Linux npx 全路径** | `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx`(MCP 必须用它,别用 Windows npx) |

## 2. 已装组件

| 类别 | 项 | 说明 |
|------|-----|------|
| 包 | `npm:@cansiny0320/pi-mcp-adapter@2.7.2` | MCP 适配器(`pi list` 可见,装于 `~/.pi/agent/npm/node_modules/`) |
| 扩展 | `~/.pi/agent/extensions/sensitive-guard.ts` | YOLO 敏感路径守卫(清单见 §4) |
| 扩展 | `plan-mode`(symlink 官方示例) | ReAct 只读计划模式,`--plan`/`/plan`、`[DONE:n]` 进度 |
| 扩展 | `subagent`(symlink 官方示例) | 并行子代理 + 链式工作流;agents 在 `~/.pi/agent/agents/` |
| Prompts | `/implement` `/scout-and-plan` `/implement-and-review` | 工作流 prompt(symlink 到 `~/.pi/agent/prompts/`) |
| Agents | `scout` `planner` `reviewer` `worker` | `~/.pi/agent/agents/*.md`,已去掉 claude 模型引用,继承 meraai |
| Skills | `unit-convert`、`json-tool` | 位于 `~/.pi/agent/skills/`,`enableSkillCommands: true` |
| MCP servers | `filesystem` + `mini` + `searxng` | 项目 `/home/user/pi_test/.mcp.json`(filesystem 用 Linux npx 全路径;mini/searxng 是本地无依赖 `mcp-servers/*.js`) |
| Web 搜索 | `searxng`(本地 SearXNG) | `mcp-servers/searxng-server.js`,`directTools: true` → 原生工具 `searxng_search`;后端 `http://localhost:11199`(`SEARXNG_URL` 可覆盖) |
| Provider | `meraai`(Mera AI) | `~/.pi/agent/models.json`,`api: anthropic-messages`,10 个模型 |
| 鉴权 | `meraai` | `~/.pi/agent/auth.json`,权限 0600 |
| 工具 | `/home/user/pi_test/tools/pi-autorun.js`、`tools/claude-autorun.mjs` | 长程自动循环:pi RPC / Claude Agent SDK,最多 N 轮,`[TASK_COMPLETE]` 提前结束 |

## 3. 配置现状(`~/.pi/agent/settings.json`)

```json
{
  "lastChangelogVersion": "0.84.3",
  "theme": "dark",
  "enableSkillCommands": true,
  "defaultProjectTrust": "always",
  "defaultProvider": "meraai",
  "defaultModel": "deepseek-v4-flash",
  "defaultThinkingLevel": "high",
  "thinkingBudgets": { "high": 32768 },
  "compaction": { "enabled": true, "reserveTokens": 200000 },
  "packages": ["npm:@cansiny0320/pi-mcp-adapter"]
}
```

- `defaultProjectTrust: "always"` = **YOLO 自动执行**(无工具确认弹窗,Pi 本身就没有;信任询问也关掉)。
- 默认模型 = `meraai/deepseek-v4-flash`,`pi` 免参数直跑。
- 模型 `deepseek-v4-flash` 的 `contextWindow` = **1000000(1M,模型真实上限)**,自动压缩阈值 = `contextWindow − compaction.reserveTokens` = **1M − 200K = 800K**(即"800K 压缩"):会话上下文超过 800K 才触发总结,给 200K 余量防后端报错;思考等级默认 `high`(`thinkingBudgets.high=32768`,实际发送被 `maxTokens−1024` 钳制)。

### 3.1 项目内配置快照(`/home/user/pi_test/config/`)

运行时配置已同步到项目:`settings.json`、`models.json`(与 `~/.pi/agent` 逐字节一致)、`extensions/`(sensitive-guard.ts + plan-mode/subagent **实体文件,已解除 symlink**)、`agents/`、`prompts/`、`skills/`。**`auth.json` 含真实密钥,永远只留在 `~/.pi/agent/`(0600),不进项目/git。**

重新同步命令(`~/.pi/agent` 改完跑一次;`-L` 解除 symlink):

```bash
mkdir -p /home/user/pi_test/config
cp ~/.pi/agent/settings.json ~/.pi/agent/models.json /home/user/pi_test/config/
cp -rL ~/.pi/agent/extensions ~/.pi/agent/agents ~/.pi/agent/prompts ~/.pi/agent/skills /home/user/pi_test/config/
```

## 4. YOLO + 敏感目录守卫

- **机制**:Pi 无 `--yolo`/`skip-permissions`,工具默认以进程权限直接执行;守卫扩展拦截敏感路径的读写。
- 拦截清单:`~/.ssh` `~/.aws` `~/.config/gcloud` `~/.kube` `~/.gnupg` `~/.netrc` `~/.git-credentials` `~/.npmrc` `~/.claude` `~/.pi/agent/auth.json` `/etc/shadow` `/etc/gshadow` `/etc/sudoers` `/etc/sudoers.d` `/etc/passwd`。
- 临时关闭:`PI_GUARD_OFF=1`。改清单:编辑 `sensitive-guard.ts` 顶部 `SENSITIVE`。

## 5. meraai provider 模型(10)

`deepseek-v4-flash`、`deepseek-v4-flash-vision-exp`、`deepseek-v4-pro`、`glm-5.2`、`glm-5.3`、`kimi-k2.7-code`、`MiniMax-M2.7`、`MiniMax-M2.7-highspeed`、`qwen3.6-flash`、`qwen3.7-max`

- 端点 `https://api.meraai.cn`;实测 `/v1/messages`(message)✅、`/v1/chat/completions` ✅、`/v1/responses` ❌ 500 → 用 `anthropic-messages`。

## 6. 能力矩阵(实测过)

| 能力 | 状态 | 验证方式 |
|------|------|----------|
| 基础问答(meraai) | ✅ | `pi -p "Reply with exactly: ok"` |
| bash/read/write 工具 | ✅ | 对话中让模型用工具 |
| MCP(filesystem + mini) | ✅ | `mcp({ })` 查状态;`mcp({ tool, args })` 直调 |
| Skills 自动发现 + `/skill:` | ✅ | `pi -p "/skill:unit-convert 0 c f"` → 0°C = 32°F |
| 敏感目录守卫 | ✅ | `read ~/.npmrc` → 被拦并返回原因 |
| 项目信任自动放行 | ✅ | `defaultProjectTrust: always` |
| 外部对接 RPC | ✅ | `pi --mode rpc` + 保持 stdin 打开的客户端 |
| SDK(`createAgentSession`) | ✅(文档化,未实测) | 见 PI_GUIDE.md §6.2 |
| ReAct 计划模式(plan-mode) | ✅ | `pi --plan -p "..."` → 只读、写被拒、产出编号 Plan |
| 并行子代理(subagent) | ✅ | `Run 2 scouts in parallel` → 2 个并发子进程都完成 |
| 链式工作流(subagent) | ✅(文档化) | `/implement` `/scout-and-plan` `/implement-and-review` |
| 长程自动循环(pi-autorun) | ✅ | 计数任务 4 轮自动完成,`[TASK_COMPLETE]` 提前结束 |
| 长程自动循环(claude-autorun) | ✅ | 计数任务 3 轮自动完成,跨轮续同一会话(SDK `resume`),`[TASK_COMPLETE]` 提前结束 |
| 调用 Claude Code(跨 Agent) | ✅ | `claude-autorun.mjs`(SDK)、`claude -p --output-format stream-json`(CLI)、`createSdkMcpServer`(MCP 化)三路已核实,细节见 `PI_AGENT_CALL.md` |
| Web 搜索(本地 SearXNG) | ✅ | `pi -p "用 searxng_search 搜索 ..."` → 模型自动发现工具并返回真实结果 |

## 7. 复验命令(装完/修完跑一遍)

```bash
node --version && npm --version && pi --version
npm config get registry                            # → npmmirror
pi list                                            # → @cansiny0320/pi-mcp-adapter
ls ~/.pi/agent/extensions/ ~/.pi/agent/agents/ ~/.pi/agent/prompts/
#   → plan-mode, subagent, sensitive-guard.ts; scout planner reviewer worker; implement*.md
cat /home/user/pi_test/.mcp.json                   # → filesystem + mini + searxng
node /home/user/pi_test/mcp-servers/searxng-server.js <<< '{"jsonrpc":"2.0","id":1,"method":"initialize"}'
pi -p "用 searxng_search 搜索「人工智能」并列出前2条"  # → 模型调用工具并返回结果
pi -p "Reply with exactly: ok"                     # → ok(默认 meraai)
pi -p "/skill:unit-convert 0 c f"                  # → 0°C = 32°F
pi --help | grep -- --plan                          # → --plan 已注册(plan-mode)
node /home/user/pi_test/tools/pi-autorun.js "示例任务" 3   # → 自动循环跑起来
```

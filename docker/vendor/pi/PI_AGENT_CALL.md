# 跨 Agent 调用:调用 Claude Code 能力

> 本文件内容为**本机实测核实**(2026-08-26,`claude` 2.1.153、`@anthropic-ai/claude-agent-sdk` 0.3.246)。回答三个问题:
> ① 怎么从外部 / pi 把任务**调进 Claude Code**;② 默认的 claude code 和 pi 两边各要**装什么、怎么协商对齐**;③ 步骤怎么走。
> 配套:`tools/claude-autorun.mjs`(SDK 封装)、`PI_CAPABILITY.md`(环境清单)。

---

## 1. 调用方式(三种,已核实)

### 1.1 Claude Agent SDK(推荐,多轮循环 / 复杂编排)
- 包:TS 用 `@anthropic-ai/claude-agent-sdk`,本机已装在 `tools/`。
- 入口:`query({ prompt, options })` 返回 async generator。
- **已核实导出**:`query`、`createSdkMcpServer`、`listSessions`、`getSessionInfo`、`forkSession`、`deleteSession`、`renameSession`、`tagSession`、`listSubagents`、`getSessionMessages`、`getSubagentMessages`、`startup`。
- **已核实选项字段**:`cwd`、`model`、`permissionMode`(同 `permission_mode`)、`maxTurns`(同 `max_turns`)、`allowedTools`、`disallowedTools`、`resume`、`includePartialMessages`、`canUseTool`(同 `can_use_tool`)。
- **已核实消息类型**:`system`(subtype `init` → 携带 `session_id` / `model` / `permissionMode`)、`assistant`(message.content 里 `type:"text"` 块)、`user`(工具结果)、`result`(本轮结束)。
- 现成封装:`tools/claude-autorun.mjs`(N 轮循环、`resume` 跨轮续同一会话、`[TASK_COMPLETE]` 提前结束),3 轮实测通过。

### 1.2 CLI 一次性 JSONL(轻量脚本 / cron)
```bash
claude -p "任务" \
  --output-format stream-json \
  --permission-mode bypassPermissions \
  [--input-format stream-json] \
  [--resume <session_id>] \
  [--allowedTools "Read,Grep,Glob,Bash"] \
  [--disallowedTools "Bash:rm -rf"]
```
- 每行一个 JSON(`system` / `assistant` / `user` / `result` / `stream_event`)。
- **坑**:WSL 下 stdin 连续发两条消息会挂起(upstream #3187),多轮请走 SDK。

### 1.3 MCP 化(把 Claude Code 暴露成 MCP server,给 pi 直调)
- **核实结论**:claude CLI **没有** `--mcp-server` flag(2.1.153);正确姿势是 SDK 的 `createSdkMcpServer({ name, version?, ... })` → 返回 MCP server 配置(已核实存在)。
- 流程:写个小脚本用 `createSdkMcpServer` 起一个 MCP stdio server → 注册进 pi 侧 `.mcp.json` → pi 用 `pi-mcp-adapter` 把 Claude Code 的能力当工具调。
- 反向也通:Claude Code 用 `--mcp-config <file>` 或项目 `.mcp.json` 连别的 MCP server(searxng 等本地 server 可直接复用)。

---

## 2. 两边安装清单

### 2.1 Claude Code 侧(默认配置)
| 项 | 值 |
|----|----|
| 安装 | `npm install -g @anthropic-ai/claude-code`(本机 `/usr/bin/claude`,v2.1.153) |
| 密钥 / 模型 | 项目 `.claude/settings.json` 的 `env`(见下) |
| 无头 flag(已核实) | `-p/--print`、`--output-format stream-json`、`--input-format stream-json`、`--permission-mode`、`--dangerously-skip-permissions`、`--allowedTools`、`--disallowedTools`、`-r/--resume`、`--from-pr`、`--mcp-config`、`mcp` 子命令 |
| ⚠️ 不存在 | `--max-turns`(CLI 无此 flag;轮次限制在 SDK 里用 `maxTurns` 选项)、`--mcp-server`(用 `createSdkMcpServer`) |

`.claude/settings.json`(env 示例,与 pi 共用同一把 meraai key / baseUrl / 模型):
```json
{
  "env": {
    "ANTHROPIC_AUTH_TOKEN": "sk-...",
    "ANTHROPIC_BASE_URL": "https://api.meraai.cn",
    "ANTHROPIC_MODEL": "deepseek-v4-flash",
    "ANTHROPIC_DEFAULT_*_MODEL": "deepseek-v4-flash"
  }
}
```
> `.claude/` 含真实密钥,已在本项目 `.gitignore` 中排除,不入库。

### 2.2 pi 侧
| 项 | 值 |
|----|----|
| 安装 / 配置快照 | 见 `PI_AGENT_INSTALL.md`(Aliyun 源、`~/.pi/agent` 快照恢复) |
| 鉴权 | `~/.pi/agent/auth.json`(0600,仅本地) |
| MCP adapter | `npm:@cansiny0320/pi-mcp-adapter`(装于 `~/.pi/agent`) |
| 对外调用方式(已核实) | `pi --mode rpc`(JSONL)、`pi -p "任务"`(一次性) |
| 会话 flag(已核实) | `-c/--continue`、`-r/--resume`、`--session`、`--session-id`、`--session-dir`、`--no-session`、`--fork` |

### 2.3 SDK 依赖(只有要走 SDK / 1.3 MCP 化才需要)
```bash
cd tools && npm install   # @anthropic-ai/claude-agent-sdk@0.3.246,一次即可
```

---

## 3. 协商 / 对齐(两边要"谈拢"的点)

| 项 | Claude Code | pi | 对齐要求 |
|----|-------------|-----|----------|
| 鉴权 | `.claude/settings.json` → `ANTHROPIC_AUTH_TOKEN` | `~/.pi/agent/auth.json` | 同一把 meraai key;base URL 都指 `https://api.meraai.cn` |
| 模型 | `ANTHROPIC_MODEL=deepseek-v4-flash` | `models.json` + `defaultModel=deepseek-v4-flash` | 同一模型 id,能力一致;pi 已去掉子代理里的 claude 模型引用 |
| 权限姿势 | `--permission-mode bypassPermissions` + allowed/disallowed | `defaultProjectTrust: always` + `sensitive-guard.ts` | 同为 YOLO 语义;敏感路径各自拦截(清单见 `PI_CAPABILITY.md` §4) |
| cwd / 配置发现 | 从 cwd 读 `.claude/settings.json` | 从 cwd 读 `.mcp.json`,全局读 `~/.pi/agent` | 脚本 `CLAUDE_CWD` 默认指向项目根,保证两边都读到各自配置 |
| 会话续接 | SDK `resume` / CLI `--resume <id>` | `-r/--resume`、`--session <id>` | 各自会话隔离,互不干扰 |

---

## 4. 实际操作步骤

1. **装两边**(§2):claude code(含 `.claude/settings.json` 三件套:token/baseUrl/model)、pi(配置快照恢复 + auth.json + mcp-adapter)。
2. **对齐**(§3):确认同一 meraai key、同一模型 `deepseek-v4-flash`、权限姿势一致。
3. **选调用方式**:
   - 长程任务编排(推荐):
     ```bash
     cd tools && npm install                       # 只需一次
     node claude-autorun.mjs "任务" 5 --out run.log  # 调 Claude Code(SDK)
     node pi-autorun.js "任务" 5                     # 调 pi(RPC)
     ```
   - 一次性调用:
     ```bash
     claude -p "任务" --output-format stream-json --permission-mode bypassPermissions
     pi -p "任务"
     ```
   - MCP 互通(进阶):用 `createSdkMcpServer` 起 Claude Code MCP server,注册进 pi 侧 `.mcp.json`,pi 里当 mcp 工具直调;反向用 `--mcp-config` 连 pi 的本地 server。
4. **复验**:`node tools/claude-autorun.mjs "Reply with exactly: ok" 1` → 输出含 `ok` 即通。

---

## 5. 已核实 vs 待做

**已核实(实测)**:SDK 导出 / 选项 / 消息类型;claude CLI flags(含"无 `--max-turns`、无 `--mcp-server`");pi CLI flags;`createSdkMcpServer` 存在及签名;`claude-autorun.mjs` 3 轮冒烟(跨轮续会话 + `[TASK_COMPLETE]` 提前结束);SDK 依赖安装。

**待做(尚未实测)**:① `createSdkMcpServer` 桥接脚本(pi → Claude Code 走 MCP,即 §1.3)还没写;② `.claude/settings.json` 的 `ANTHROPIC_DEFAULT_*_MODEL` 通配键在 SDK 驱动下是否继承未单独验证(继承环境即沿用)。需要时下一步补齐。

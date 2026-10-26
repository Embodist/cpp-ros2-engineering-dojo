# Pi 进阶指南:模型配置 / 启动 / MCP / Skills / 插件 / 外部管理对接

> 配套文档:
> - `PI_USAGE.md` — 基础使用(已含 Mera AI provider 的配置与验证)
> - `PI_INSTALL.md` — **从零安装 Pi 及核心插件(Aliyun Node 源)** ← 安装/装机看这个
> - `PI_AGENT_INSTALL.md` — **给 AI agent 照着装的速查版**(关键问题+应对 harness) ← 机器人看这个
> - `PI_CAPABILITY.md` — **本机能力清单**(已装组件/配置/复验命令) ← 体检/状态看这个
> - `PI_INSTALL_PROMPT.md` — **给 AI 的一键安装 prompt**(复制即用) ← 想让别的 AI 装就看这个
> - 本文档 — 进阶:自定义模型、多种启动方式、如何接入 MCP / Skills / 插件(Packages),以及如何让别的应用**监控 Pi 的执行并交互**(RPC / SDK)
> 官方文档位于 npm 包内:`docs/{models,skills,extensions,packages,rpc,sdk}.md`。

---

## 1. 自定义模型配置

Pi 的 provider 分两层:**内置 provider**(anthropic / openai / google / deepseek …)和**自定义 provider**。自定义有两条路:

### 1.1 方式 A:`~/.pi/agent/models.json`(声明式,零代码)

适合对接任何支持以下协议的端点:

| `api` 值 | 系列 | 适用 |
|----------|------|------|
| `anthropic-messages` | **message 系列** | Anthropic Messages API 及其兼容后端 |
| `openai-completions` | **chat_completion 系列** | OpenAI Chat Completions 及其兼容后端 |
| `openai-responses` | **response 系列** | OpenAI Responses API |
| `google-generative-ai` | — | Google Generative AI |
| `azure-openai-responses` / `openai-codex-responses` / `mistral-conversations` / `google-vertex` / `bedrock-converse-stream` | — | 各专用 API |

> **选哪个系列?** 先探测后端支持哪种协议。对 `api.meraai.cn` 实测:`/v1/messages`(message)✅、`/v1/chat/completions`(chat_completion)✅、`/v1/responses`(response)❌ 500 not implemented → 本机选择 **`anthropic-messages`(message 系列)**,详见 `PI_USAGE.md` §3.1。

**最小示例(本地 Ollama 风格):**

```jsonc
// ~/.pi/agent/models.json
{
  "providers": {
    "my-llm": {
      "name": "My LLM",
      "baseUrl": "http://localhost:1234/v1",
      "api": "openai-completions",
      "apiKey": "$MY_LLM_API_KEY",       // 见下方取值语法
      "models": [
        { "id": "my-model", "reasoning": true, "input": ["text"], "contextWindow": 128000, "maxTokens": 16384 }
      ]
    }
  }
}
```

**关键字段:**

- `baseUrl`:根地址即可,Pi 自动拼协议路径(`anthropic-messages` → `/v1/messages`)。
- `apiKey` / `headers` 取值语法(与官方 models.md 一致):
  - `!命令` → 整段作为 shell 命令执行,取 stdout(如 `!op read 'op://vault/item/key'`)
  - `$VAR` / `${VAR}` → 环境变量插值
  - `$$` 转义字面 `$`,`$!` 转义字面 `!`
  - 其余 → 字面量
- 模型级可覆盖:`api`、`baseUrl`、`reasoning`、`thinkingLevelMap`、`input`、`contextWindow`、`maxTokens`、`cost`、`headers`、`compat`、`samplingParams`。
- `compat`:OpenAI 兼容后端的怪癖开关。例:`supportsDeveloperRole: false`(用 `system` 代替 `developer`)、`thinkingFormat: "deepseek"`、`maxTokensField: "max_tokens"`、`cacheControlFormat: "anthropic"`。
- **鉴权解析顺序**(官方 providers.md):CLI `--api-key` → `~/.pi/agent/auth.json` → 环境变量 → `models.json` 的 `apiKey`。
- 文件在 `/model` 打开时会热加载,不用重启。

**示例:`~/.pi/agent/auth.json`**

```json
{ "meraai": { "type": "api_key", "key": "sk-..." } }
```

### 1.2 方式 B:Extension 注册 `pi.registerProvider()`(需代码)

需要自定义流式、OAuth/SSO、动态拉模型时用。文件放 `~/.pi/agent/extensions/` 即可自动发现:

```typescript
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";

export default async function (pi: ExtensionAPI) {
  // 动态从远端拉模型清单(启动前完成,`pi --list-models` 即可见)
  const res = await fetch("http://localhost:1234/v1/models");
  const data = (await res.json()) as { data: { id: string }[] };

  pi.registerProvider("local-openai", {
    baseUrl: "http://localhost:1234/v1",
    apiKey: "$LOCAL_KEY",
    api: "openai-completions",
    models: data.data.map((m) => ({
      id: m.id, reasoning: false, input: ["text"],
      contextWindow: 128000, maxTokens: 4096,
    })),
  });
}
```

覆盖已有 provider(如把 anthropic 全部请求代理到网关):

```typescript
pi.registerProvider("anthropic", { baseUrl: "https://proxy.example.com" });
```

### 1.3 校验

```bash
pi --list-models                # 列出所有 provider 的模型
pi --list-models meraai         # 只看 meraai
pi auth                         # 查看 provider 凭据就绪状态
```

---

## 2. 启动方法

```bash
# 交互 TUI(在有 TTY 的终端)
cd /home/user/pi_test
pi

# 指定 provider / 模型 / 思考等级
pi --provider meraai --model deepseek-v4-flash --thinking high

# 非交互一次性(print 模式)
pi -p "总结这个仓库"
echo "text" | pi -p "总结"
pi @README.md "Review this"

# JSON 事件输出(一次性,便于程序解析)
pi -p "hello" --mode json

# RPC 模式(长期进程,JSONL 协议,供外部应用对接,见 §6)
pi --mode rpc --provider meraai --model deepseek-v4-flash

# 会话管理
pi -c                          # 继续最近会话
pi -r                          # 浏览历史会话
pi --name "任务名"              # 命名会话
pi --session <path|id>         # 打开指定会话
pi --no-session                # 不落盘(临时)
pi --session-dir <dir>         # 自定义会话目录

# 加载扩展 / skills(临时,不安装)
pi -e ./my-extension.ts
pi -e npm:pi-mcp-extension     # 临时试用某个包
pi --skill ./my-skill/

# 其他常用
pi --list-models               # 列模型
pi --offline                   # 跳过启动联网(拉目录等)
pi --no-extensions             # 禁用扩展自动发现
pi --no-skills                 # 禁用 skills 发现
```

环境变量影响:Pi 也读 `~/.pi/agent/AGENTS.md`(全局上下文)、项目 `AGENTS.md` / `CLAUDE.md`。改动后 `/reload` 或重启生效。

### 2.1 YOLO 模式(默认最高权限执行)

> **Pi 0.84.3 没有 `--yolo` 标志,也**没有内置的工具执行确认弹窗**——内置工具(`read`/`bash`/`edit`/`write`/`grep`/`find`/`ls`)以进程权限**直接执行**,无沙箱、无 confirm 门禁(官方 security.md 明示)。所以"最高权限"默认就是事实;剩下的唯一"询问"是**项目信任**(`defaultProjectTrust`,默认 `ask`)。**开 yolo = 把项目信任设为永远自动放行。**

**永久开启(推荐,本机已配置):**

```jsonc
// ~/.pi/agent/settings.json
{
  "defaultProjectTrust": "always",   // ask | always | never(仅全局设置,项目级无此键)
  "defaultProvider": "meraai",       // 让 `pi` 免参数直跑
  "defaultModel": "deepseek-v4-flash"
}
```

**单次运行 / 补充手段:**

```bash
pi --approve -p "..."        # -a / --approve:本次信任项目资源
pi -p "..."                  # 非交互(-p / --mode json / --mode rpc)本来就不弹信任框
# 交互内保存信任决定:输入 /trust  → 写 ~/.pi/agent/trust.json(重启生效)
```

**敏感目录例外(本机已装守卫):** 默认全自动执行,但**读取/写入敏感路径会被拦截**。由扩展
`~/.pi/agent/extensions/sensitive-guard.ts` 实现(实测:`read ~/.npmrc` 返回拦截原因;正常读项目文件不受影响)。
拦截清单(`.ssh`/`.aws`/`.config/gcloud`/`.kube`/`.gnupg`/`.netrc`/`.git-credentials`/`.npmrc`/`.claude`/`~/.pi/agent/auth.json`、`/etc/shadow` 等)与临时开关 `PI_GUARD_OFF=1` 都写在文件顶部,按需改。

### 2.2 权限限定(默认放开、配置收窄)

"默认最高权限,除非配置文件限定"——用下面这些把权限收窄:

| 手段 | 作用 |
|------|------|
| `"defaultTools": ["read","grep","find","ls"]` | 全局默认只开这些内置工具(空数组 = 全关) |
| `--tools read,bash,edit` / `--exclude-tools write` | 单次允许 / 排除工具(含扩展与自定义工具) |
| `.pi/settings.json` 覆盖全局 | 项目级改模型 / 默认工具 / 关 skills 等 |
| `defaultProjectTrust: "never"` | 反过来:永不自动信任项目资源 |
| Extension `tool_call` 事件 | 自建门禁/守卫。**本机已装 `sensitive-guard.ts`**:默认 yolo 只拦敏感路径;要拦截更多(如 `rm -rf`、`sudo`)照它的写法加判断(见 §5.1 示例) |
| 容器 / Gondolin 沙箱 | 真正的系统级隔离(见 npm 包内 `docs/containerization.md`) |

> 注意:Pi 的"信任"只控制**加载项目资源**(`.pi/settings.json`、项目 skills/extensions),不是工具执行权限。要真隔离未可信代码,请用容器,不要把"信任=always"当沙箱。

---

## 3. 扩展 Skills

Skills 是**按需加载**的能力包,遵循 [Agent Skills 标准](https://agentskills.io/specification)。Pi 没有内置 MCP,但 Skills 是内建一等公民。

### 3.1 发现位置

- 全局:`~/.pi/agent/skills/`、`~/.agents/skills/`
- 项目(需信任):`.pi/skills/`、`.agents/skills/`(cwd 及祖先目录)
- 包:`skills/` 目录或 package.json 的 `pi.skills`
- 设置:`settings.json` 的 `skills` 数组
- CLI:`--skill <path>`(可重复)

**复用 Claude Code 的 skills:**

```jsonc
// ~/.pi/agent/settings.json
{ "skills": ["~/.claude/skills"] }
```

### 3.2 结构

一个 skill = 一个目录 + `SKILL.md`:

```
my-skill/
├── SKILL.md          # frontmatter + 指令(必需)
├── scripts/          # 辅助脚本
└── references/       # 按需加载的详细文档
```

`SKILL.md` frontmatter(`name`、`description` 必填):

```markdown
---
name: my-skill
description: 处理 PDF:抽取文本、填表、合并。处理 PDF 文档时使用。
---
# My Skill
用法说明……(用相对路径引用 scripts/)
```

### 3.3 使用

```text
/skill:my-skill             # 显式加载执行
/skill:my-skill 参数         # 带参数(追加为 User: <args>)
```

默认 `enableSkillCommands` 开关:`/settings` 里开,或 settings.json 设 `"enableSkillCommands": true`。模型也可在匹配任务时自动用 `read` 加载 skill(渐进式披露:上下文只带描述)。

### 3.4 参考仓库

- [Anthropic Skills](https://github.com/anthropics/skills)
- [Pi Skills](https://github.com/badlogic/pi-skills)

> **本机已安装**(`~/.pi/agent/skills/`,已实测):
> - `unit-convert` — 单位换算(摄氏/华氏、km/mi、kg/lb),脚本 `scripts/convert.sh`
> - `json-tool` — JSON 校验 / 美化 / 按路径取值,脚本 `scripts/json-tool.js`
>
> 已在 `~/.pi/agent/settings.json` 开启 `"enableSkillCommands": true`,支持 `/skill:unit-convert 0 c f` 带参调用。
> 由于本机 GitHub 不可达,未安装 anthropics/skills 等大仓库;网络恢复后可 clone 到 `~/.agents/skills/` 或在 settings.json 的 `skills` 数组里加路径复用。

---

## 4. 扩展 MCP

> **重要:Pi 刻意不内置 MCP**(官方 usage.md 明示)。接入 MCP 用**扩展/包**方式。

社区成熟方案(任选其一,均为 `pi install` 可装):

| 包 | 说明 |
|----|------|
| `npm:@cansiny0320/pi-mcp-adapter` | 社区标注的"官方/标准"MCP 适配器 |
| `npm:pi-mcp-extension` | 支持 stdio / streamable-http / sse,自动重连、工具热刷新 |
| `git:github.com/tickernelz/pi-mcp-tools` | 提供 `/mcp-status`、`/mcp-list`、`/mcp-toggle` 等命令 |
| `pi-mcp-adapter` 的替代 | 因 Zod v4/v3 冲突,部分人改用绕过 MCP SDK 直连 JSON-RPC 的实现 |

> ⚠️ **已知坑**:Pi 内置 Zod v4,而 MCP SDK 1.x 依赖 Zod v3,可能报 `v3Schema.safeParse is not a function`。若安装后工具不出现,换一个实现(或选不依赖 MCP SDK 的方案)。`@cansiny0320/pi-mcp-adapter@2.7.2` 显式支持 Zod v4(`^3.25.0 || ^4.0.0`),无此问题。

> ✅ **本机已安装并实测**:`npm:@cansiny0320/pi-mcp-adapter@2.7.2`(`pi install npm:@cansiny0320/pi-mcp-adapter`)。配置在项目 `.mcp.json`,实测两种 MCP server 均可调通(见 §9)。
> ⚠️ **WSL/Windows 混用的 npx 坑**:本机 `which npx` 指向 `/mnt/d/Program Files/Nodejs/...`(Windows 版),在 WSL 里跑会报 `ERR_INVALID_URL`。MCP 配置里务必写 Linux npx 的**全路径**(本机为 `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx`)。

### 4.1 标准接入步骤(本机采用的 `@cansiny0320/pi-mcp-adapter`)

```bash
pi install npm:@cansiny0320/pi-mcp-adapter
```

配置(adapter 读取顺序:项目 `.mcp.json` > `~/.config/mcp/mcp.json` > `.pi/mcp.json` > `~/.pi/agent/mcp.json`):

```jsonc
// /home/user/pi_test/.mcp.json
{
  "mcpServers": {
    "filesystem": {
      "command": "/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx",  // Linux npx 全路径
      "args": ["-y", "@modelcontextprotocol/server-filesystem@latest", "/tmp/pi_mcp_test"]
    },
    "mini": {
      "command": "node",
      "args": ["/home/user/pi_test/mcp-servers/mini-server.js"]          // 本地自定义 server
    }
  }
}
```

重启 Pi(或 `/reload`)后即可用。工具默认通过 `mcp()` 总工具暴露(`mcp({ })` 看状态、`mcp({ server })` 列工具、`mcp({ tool, args })` 直调),也可给单个 server 设 `"directTools": true` 把每个 MCP 工具注册为原生 Pi 工具(如 `mini_add`)。`/mcp` 命令的 status/panel 需要 TUI,headless(RPC/`-p`)下看不到,请用 `mcp({ })` 工具查询。

### 4.2 自建:用 Extension 连接任意 MCP server

Pi 提供 `pi.registerTool()`,你可以在扩展里用 `@modelcontextprotocol/sdk` 连 MCP server、把工具注册进来:

```typescript
import { Client } from "@modelcontextprotocol/sdk/client/index.js";
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";

export default function (pi: ExtensionAPI) {
  // 伪代码:连接、tools/list、逐个 registerTool
}
```

### 4.3 安全提示

MCP server 有完整系统权限,只连可信源;`headers` 里的 token 用 `$VAR` 引用避免明文。

---

## 5. 插件:Extensions 与 Packages

Pi 的"插件"= **Extension**(代码)+ **Package**(分发单元,可捆绑 extension / skill / prompt / theme)。

### 5.1 Extension:自动发现与热重载

- 自动发现目录(文件放入即可,`/reload` 热重载):
  - 全局:`~/.pi/agent/extensions/*.ts`、`~/.pi/agent/extensions/*/index.ts`
  - 项目(需信任):`.pi/extensions/*.ts`、`.pi/extensions/*/index.ts`
- 临时:`pi -e ./my.ts`
- 更多路径:settings.json 的 `extensions` 数组。

**最小扩展**(事件 + 工具 + 命令):

```typescript
import type { ExtensionAPI } from "@earendil-works/pi-coding-agent";
import { Type } from "typebox";

export default function (pi: ExtensionAPI) {
  pi.on("tool_call", async (event, ctx) => {
    if (event.toolName === "bash" && event.input.command?.includes("rm -rf")) {
      const ok = await ctx.ui.confirm("危险操作", "允许 rm -rf?");
      if (!ok) return { block: true, reason: "用户拒绝" };
    }
  });

  pi.registerTool({
    name: "greet",
    description: "打招呼",
    args: Type.Object({ name: Type.String() }),
    async execute({ name }) {
      return `你好,${name}!`;
    },
  });

  pi.registerCommand("hello", {
    description: "测试命令",
    async run(_, ctx) { ctx.ui.notify("hello!", "info"); },
  });
}
```

**ExtensionAPI 核心能力**:`pi.registerTool` / `pi.registerCommand` / `pi.registerProvider` / `pi.on(事件)` / `pi.unregisterProvider` / `pi.appendEntry()`(持久化状态)/ `ctx.ui`(select/confirm/input/notify/custom 自定义 UI)。

**常用事件**(用于拦截/增强/监控,详见 §6.4):`session_start`、`tool_call`、`tool_result`、`message_end`、`turn_end`、`agent_end`、`agent_settled`、`before_provider_request`、`after_provider_response`、`model_select` 等。

### 5.2 Package:安装与分发

```bash
pi install npm:@scope/pkg@1.0.0    # npm 源
pi install git:github.com/user/repo@v1   # git 源
pi install /path/to/dir            # 本地目录
pi remove npm:@scope/pkg
pi list                            # 列出已装
pi update                          # 更新 pi 本身
pi update --all                    # 更新 pi + 包 + 对账 git refs
pi config                          # TUI 启用/禁用包内资源(Tab 切换全局/项目)
pi config -l                       # 从项目本地开始
```

- `-l` 写入项目设置 `.pi/settings.json`(可团队共享,启动时自动补装);默认写全局 `~/.pi/agent/settings.json`。
- 包声明资源:`package.json` 里 `"pi": { "extensions": [...], "skills": [...], "prompts": [...], "themes": [...] }`,或约定目录(`extensions/`、`skills/`、`prompts/`、`themes/`)。
- 包以全系统权限运行,安装前审阅源码。

---

## 6. 外部管理对接:让别的应用监控 Pi 并交互

Pi 提供三条外部对接通道:**RPC 模式**(JSONL 子进程)、**SDK**(Node 内嵌)、**Extension 事件**(进程内钩子)。按你的对接方选:

| 方案 | 适用 | 特点 |
|------|------|------|
| `pi --mode rpc` | 任何语言,子进程 | stdin/stdout JSONL,协议稳定,无需安装 npm 依赖 |
| `createAgentSession()`(SDK) | Node/TS 应用 | 进程内嵌入,直接拿 `AgentSession` 对象,事件订阅 |
| Extension `pi.on(...)` | 想在 Pi 进程内做监控/钩子 | 可改消息、拦截工具、注入上下文 |

### 6.1 RPC 模式(推荐用于"别的应用监控 + 交互")

启动:

```bash
pi --mode rpc --provider meraai --model deepseek-v4-flash
```

**协议**:stdin 发命令、stdout 流式回事件,每行一条 JSON(JSONL,只用 `\n` 分隔)。

**命令**(`{"type": "...", ...}`,可选 `id` 关联请求/响应):

| 命令 | 作用 |
|------|------|
| `prompt` | 发消息(`streamingBehavior: "steer"` 排队插入 / `"followUp"` 等本轮完成) |
| `steer` / `follow_up` | 立即改方向 / 排队追加 |
| `abort` | 中止当前生成 |
| `new_session` | 开新会话(可带 `parentSession`) |
| `switch_session` | 切换会话 |
| `get_state` / `get_messages` / `get_session_stats` | 读取状态/消息/统计 |
| `set_model` / `cycle_model` / `get_available_models` | 模型控制与枚举 |
| `set_thinking_level` / `cycle_thinking_level` / `get_available_thinking_levels` | 思考等级控制 |
| `set_steering_mode` / `set_follow_up_mode` | 排队模式(`one-at-a-time`) |
| `compact` | 手动压缩上下文 |
| `set_auto_compaction` / `set_auto_retry` / `abort_retry` | 自动压缩/重试开关 |
| `bash` / `abort_bash` | 让 Pi 直接执行 bash(事件流回报输出) |
| `export_html` | 导出会话为 HTML |

**事件**(stdout 实时推送,监控用):

`agent_start` / `agent_end` / `agent_settled` / `turn_start` / `turn_end` / `message_start` / `message_update`(文本/思考/toolcall 增量) / `message_end` / `tool_execution_start` / `tool_execution_update` / `tool_execution_end` / `bash_execution_update` / `queue_update` / `compaction_start` / `compaction_end` / `auto_retry_start` / `auto_retry_end` / `extension_error`

**示例客户端(Node.js,监控 + 提问):**

```javascript
const { spawn } = require("node:child_process");
const rl = require("node:readline");

const pi = spawn("pi", ["--mode", "rpc", "--provider", "meraai", "--model", "deepseek-v4-flash"]);
const r = rl.createInterface({ input: pi.stdout });

r.on("line", (line) => {
  const ev = JSON.parse(line);
  if (ev.type === "message_update" && ev.assistantMessageEvent?.type === "text_delta")
    process.stdout.write(ev.assistantMessageEvent.delta);
  if (ev.type === "agent_settled") {
    console.log("\n[监控] 本轮结束,可发下一条。");
    pi.stdin.write(JSON.stringify({ type: "prompt", message: "继续", streamingBehavior: "followUp" }) + "\n");
  }
});

pi.stdin.write(JSON.stringify({ type: "prompt", message: "列出当前目录文件" }) + "\n");
```

> 完整协议、字段、错误处理见官方 `docs/rpc.md`。

### 6.2 SDK:Node/TS 进程内嵌入

```typescript
import { createAgentSession } from "@earendil-works/pi-coding-agent";

const { session } = await createAgentSession({
  cwd: "/home/user/pi_test",
  provider: "meraai", model: "deepseek-v4-flash",
});

// 监控:订阅事件
session.subscribe((event) => {
  if (event.type === "message_update" && event.assistantMessageEvent.type === "text_delta")
    process.stdout.write(event.assistantMessageEvent.delta);
  if (event.type === "tool_execution_end") console.log(`\n[工具] ${event.toolName} 完成`);
  if (event.type === "agent_settled") console.log("\n[完成] 代理已静默");
});

// 交互
await session.prompt("读一下 README 并总结");
await session.prompt("停,先做别的", { streamingBehavior: "steer" });
await session.followUp("结束后再检查 X");
await session.agent.waitForIdle();   // 等待完全空闲
```

- `createAgentSession()` 可选传 `provider`、`model`、`systemPrompt` 等;也可用 `createAgentSessionRuntime()` 管理换会话(新/恢复/fork)。
- 事件订阅与 RPC 事件类型一致,另加 `message_start`/`message_end` 携带完整 `AgentMessage`。

### 6.3 会话文件(被动监控/审计)

会话默认存 `~/.pi/agent/sessions/<项目名>/<会话>.jsonl`,逐行追加事件 JSON。外部应用可以 tail 这个文件做审计,或用 `pi --mode json -p` 一次性拿全量 JSON 事件流。`--session-dir` 可自定义目录。

### 6.4 Extension 事件钩子(进程内监控/改造)

想在同一进程里响应 Pi 行为并干预(拦截危险命令、改消息、注入上下文),用扩展:

```typescript
pi.on("agent_settled", (ev, ctx) => {
  // 通知外部系统(webhook / MQ / 日志)
  fetch("http://monitor.local/pi", { method: "POST", body: JSON.stringify({ state: "settled" }) });
});
```

事件可返回修改结果(`tool_call` 可 block、`message_end` 可改写消息),这是"外部管理"中**拦截/治理**能力的入口。官方 `docs/extensions.md` 有全部事件与 `ExtensionContext`。

---

## 7. 长程与自主任务(parallel work / 一直执行 / ReAct 计划)

> 对应 oh-my-codex / oh-my-claude-code 的能力。Pi 原生无 `--yolo` 类开关,但这几类"自主执行"都有现成实现(均来自官方自带示例 + 一个自研 harness)。已全部在本机装好并实测(见 §9)。

### 7.1 ReAct 计划模式 → `plan-mode` 扩展(官方示例,已装)

```bash
pi --plan -p "分析 X 并给出计划"     # --plan 或交互内 /plan,只读
```

- **只读**:`edit`/`write` 工具禁用,bash 走白名单(只放行 cat/head/grep/find/ls/git status 等读操作)。
- **计划提取**:模型输出 `Plan:` 编号列表,执行阶段用 `[DONE:n]` 标记进度,`/todos` 查看。
- 交互快捷键 `Ctrl+Alt+P` 切换;`--plan` 标志由该扩展注册(`pi --help` 可见)。
- 实现位于 `~/.pi/agent/extensions/plan-mode/`(symlink 到 npm 包内示例)。

### 7.2 并行工作 / 链式工作流 → `subagent` 扩展(官方示例,已装)

```text
# 并行(最多 8 个任务、4 并发)
Run 2 scouts in parallel: one to list files, one to report pi version

# 链式(scout → planner → worker / worker → reviewer → worker)
/implement 为 session store 加 Redis 缓存
/scout-and-plan 重构 auth 支持 OAuth
/implement-and-review 给 API 加输入校验
```

- 内置子代理:**scout**(侦查)/ **planner**(计划)/ **reviewer**(评审)/ **worker**(执行),定义在 `~/.pi/agent/agents/*.md`(YAML frontmatter:`name`/`description`/`tools`,可加 `model`;省略 `model` 则继承父会话模型——本机已去掉 claude 模型引用,统一继承 meraai/deepseek-v4-flash)。
- 每个子代理 = 独立 `pi --mode json -p` 子进程,**隔离上下文**;并行时流式显示各任务进度;链式用 `{previous}` 传上一阶段结果。
- 工作流 prompt 已装到 `~/.pi/agent/prompts/`(`/implement` 等)。
- 安全:默认只加载用户级 agents;项目级 `.pi/agents/*.md` 需 `agentScope` 开启(不信任的仓库别开)。

### 7.3 长程自动循环(一直执行到 N 轮)→ `tools/pi-autorun.js`(自研)

```bash
node /home/user/pi_test/tools/pi-autorun.js "任务描述" [最大轮数] [--out 日志]
# 例:node tools/pi-autorun.js "逐步重构 X,直到所有测试通过" 10 --out autorun.log
# env:PI_PROVIDER / PI_MODEL 可换默认 meraai / deepseek-v4-flash
```

- **机制**:RPC 模式起 pi,每轮发 prompt(带轮次预算 + 上一轮输出尾部作为进度反馈),等 `agent_settled` 再发下一轮;回复**单独成行**出现 `[TASK_COMPLETE]` 即提前结束,否则跑到 `最大轮数`。
- **自动优化**:每轮把上轮结果喂回,模型据此决定继续/修正,实现"自己推进、自己调整"。
- 输出:每轮摘要 + 末尾汇总,`--out` 写日志文件。
- 这个 harness 也是 §6 RPC 对接的实例用法。

### 7.4 原生辅助:auto-retry + auto-compaction

长任务单次会话里 Pi 会自动**重试**临时错误(`retry.enabled: true`,默认 3 次)并**压缩长上下文**(`compaction.enabled: true`),不需要外部循环也能跑很久。要手动触发压缩:`/compact` 或 RPC 的 `compact` 命令。

---

## 8. 安全提醒

- Extensions / Packages / MCP server 都以**完整系统权限**运行,安装前审阅源码与配置。
- Skills 可指示模型执行任意动作(含执行脚本),启用前检查 `SKILL.md`。
- `auth.json` 为 0600 权限,勿把 token 写进普通文件或提交到 git。

---

## 9. 本机已完成的验证

**基础链路(meraai provider)**

```bash
pi --provider meraai --model deepseek-v4-flash -p "Reply with exactly the word: hello"  # → hello
pi --provider meraai --model deepseek-v4-flash -p "用 bash 列出当前目录文件并报告数量"   # → 正确调用工具
pi --list-models meraai                                                              # → 10 个模型
```

**MCP(已装 `@cansiny0320/pi-mcp-adapter`,配置见 `.mcp.json`)**

- ✅ `filesystem`(官方 npx server)+ `mini`(本地自定义,见 `mcp-servers/mini-server.js`)双服务器
- ✅ 端到端调用:`mini_add(20,22)→42`、`mini_echo('multi-server-ok')→multi-server-ok`、`filesystem_read_text_file(hello.txt)→test-content-123`
- ✅ `mcp()` 总工具状态/搜索/直调:`mcp({ })` 显示 `0/1 servers, 14 tools` 及工具清单;lazy 连接自动 `mcp({ connect: "mini" })`
- ✅ `directTools: true` 模式下 MCP 工具注册为原生 Pi 工具(`mini_add`、`mini_echo` 直接调用成功)
- ⚠️ WSL 下必须用 Linux npx 全路径;`/mcp` status 面板是 TUI 功能,headless 下用 `mcp({ })` 工具替代

**Skills(已装 `unit-convert`、`json-tool`,`enableSkillCommands: true`)**

- ✅ 模型自动发现:`98 摄氏→华氏` 时自动 `read` SKILL.md 并跑脚本,得 `208.40 °F`
- ✅ `/skill:unit-convert` 显式加载;`/skill:unit-convert 0 c f` → `0°C = 32°F`
- ✅ json-tool 端到端:validate→`valid`、pretty→格式化 JSON、get `versions[1]`→`2`

**RPC(外部对接)**

- ✅ `--mode rpc` 事件流完整:`message_update(thinking/text)` → `turn_end` → `agent_settled`;客户端需保持 stdin 打开。

**长程与自主任务(§7 的三种能力)**

- ✅ **plan-mode**:`pi --plan` 只读模式生效——尝试写文件被拒(文件未创建),模型改为输出编号 `Plan:`;`--plan` 标志已注册(`pi --help` 可见)
- ✅ **subagent 并行**:`Run 2 scouts in parallel` → 两个独立 pi 子进程并发完成(一个列 .md 文件 6 个,一个报 pi 版本 0.84.3);agents 继承 meraai/deepseek-v4-flash
- ✅ **pi-autorun 自动循环**:计数任务(0→1→2→3)跑满 4 轮,`[TASK_COMPLETE]` 单独成行时提前结束;未完成的轮次正确反馈进度继续推进
- ✅ 守卫对子代理同样生效(子进程也加载全局扩展)

# Pi 安装速查 —— 面向 AI Agent 的安装指引

> **读者对象**:Claude Code / Codex / OpenCode 等 AI agent。读完本文可直接照着执行,把 Pi 及其核心插件装上并跑通冒烟测试。
> **适用环境**:Linux / WSL(本机为 WSL2,Node 装在 `/usr/local/node-v22.22.2-linux-x64-glibc-217/`,无 sudo 时用该路径)。
> **安装源**:npm 使用 **Aliyun 镜像** `https://registry.npmmirror.com`。
> 详细参考:`PI_INSTALL.md`(安装)、`PI_GUIDE.md`(MCP/Skills/外部对接)、`PI_USAGE.md`(meraai provider 配置)、`PI_CAPABILITY.md`(本机能力清单/目标状态)、`PI_INSTALL_PROMPT.md`(给 AI 的一键安装 prompt)。

---

## 0. 一页 TL;DR(照抄即可)

```bash
# 1) 设 Aliyun 源
npm config set registry https://registry.npmmirror.com

# 2) 装 pi
npm install -g --ignore-scripts @earendil-works/pi-coding-agent

# 3) 装核心插件(MCP 适配器)
pi install npm:@cansiny0320/pi-mcp-adapter

# 4) 写 MCP 配置(路径照抄本机值)
mkdir -p /home/user/pi_test/mcp-servers
#   创建 /home/user/pi_test/.mcp.json,见 §4.1

# 5) 配 provider + 鉴权(meraai 示例)
cat > ~/.pi/agent/models.json <<'EOF'
{ "providers": { "meraai": { "name": "Mera AI", "baseUrl": "https://api.meraai.cn",
  "api": "anthropic-messages", "apiKey": "$MERAAI_API_KEY",
  "models": [ { "id": "deepseek-v4-flash", "reasoning": true, "input": ["text"],
                "contextWindow": 131072, "maxTokens": 16384 } ] } } }
EOF
cat > ~/.pi/agent/auth.json <<'EOF'
{ "meraai": { "type": "api_key", "key": "sk-YOUR_KEY" } }
EOF
chmod 600 ~/.pi/agent/auth.json

# 6) 冒烟
pi --version
pi --provider meraai --model deepseek-v4-flash -p "Reply with exactly: ok"
```

**约 5 分钟跑通。** 下面是每一步的说明与坑位。

---

## 1. 前置检查

```bash
node --version        # 需要 ≥ 20。本机 22.22.2
npm --version         # 10.9.7
npm config get registry   # 必须是 https://registry.npmmirror.com(Aliyun),否则国内网络慢/超时
which pi              # 若已装过则显示路径
```

- npm 全局根目录 = `npm prefix -g`(本机 `/usr/local/node-v22.22.2-linux-x64-glibc-217`)。
- **WSL 重点**:`which npx` 可能指向 **Windows 版** npx(`/mnt/d/Program Files/Nodejs/...`),在 WSL 里跑会 `ERR_INVALID_URL`。任何要 spawn `npx` 的场景(MCP 配置、脚本)一律写 Linux npx 全路径:
  `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx`
- 装完 pi 后若 `pi` 命令不存在,检查 `npm prefix -g` 下的 `bin/pi` 是否在 `$PATH`。

---

## 2. 安装 pi-agent

```bash
npm install -g --ignore-scripts @earendil-works/pi-coding-agent
# 固定版本:npm install -g @earendil-works/pi-coding-agent@0.84.3
```

- `--ignore-scripts` 是官方推荐(跳过依赖生命周期脚本)。
- 验证:

```bash
pi --version          # → 0.84.3
pi --help
pi --list-models      # 内置 provider 模型清单
```

---

## 3. 初始化配置目录(可选,首跑自动创建)

```bash
mkdir -p ~/.pi/agent/skills ~/.pi/agent/extensions ~/.pi/agent/sessions
chmod 700 ~/.pi/agent
```

| 文件 | 作用 |
|------|------|
| `~/.pi/agent/settings.json` | 全局设置:`theme`、`packages`、`enableSkillCommands` |
| `~/.pi/agent/auth.json` | 鉴权凭据,权限 **0600** |
| `~/.pi/agent/models.json` | 自定义 provider/模型(声明式) |
| `~/.pi/agent/extensions/` | 全局扩展,`*.ts` 自动发现,`/reload` 热重载 |
| `~/.pi/agent/skills/` | 全局 skills(`SKILL.md`) |
| `~/.pi/agent/sessions/` | 会话 JSONL(可 tail 做审计) |

---

## 4. 核心插件安装

### 4.1 MCP 适配器(**核心**)

Pi **刻意不内置 MCP**,必须装扩展包。本机选用 `@cansiny0320/pi-mcp-adapter`(显式兼容 Zod v4,避开 Pi 内置 Zod v4 与 MCP SDK 的 Zod v3 冲突——见 §5 关键问题 P2):

```bash
pi install npm:@cansiny0320/pi-mcp-adapter
```

配置(项目级 `.mcp.json`;adapter 读取顺序:项目 `.mcp.json` > `~/.config/mcp/mcp.json` > `.pi/mcp.json` > `~/.pi/agent/mcp.json`):

```jsonc
// /home/user/pi_test/.mcp.json
{
  "mcpServers": {
    "filesystem": {
      "command": "/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx",
      "args": ["-y", "@modelcontextprotocol/server-filesystem@latest", "/tmp/pi_mcp_test"]
    },
    "mini": {
      "command": "node",
      "args": ["/home/user/pi_test/mcp-servers/mini-server.js"]
    },
    "searxng": {
      "command": "node",
      "args": ["/home/user/pi_test/mcp-servers/searxng-server.js"],
      "directTools": true,
      "env": { "SEARXNG_URL": "http://localhost:11199" }
    }
  }
}
```

> 本地无依赖 server 模式(`mini`/`searxng`)绕开 npm 包的 Zod 兼容问题(P2),`node` 直接跑脚本。Web 搜索:本地 SearXNG JSON API(`http://localhost:11199/search?format=json`),`directTools: true` 注册为原生工具 `searxng_search`。

重启 Pi(或 `/reload`)后生效。用法:
- `mcp({ })` → 总状态/工具清单
- `mcp({ server })` → 列出某 server 工具
- `mcp({ tool, args })` → 直调
- 给单个 server 加 `"directTools": true` → 工具注册为原生 Pi 工具(如 `mini_add`)

### 4.2 Skills

```bash
mkdir -p ~/.pi/agent/skills/<skill-name>/scripts
```

每个 skill = 目录 + `SKILL.md`(frontmatter:`name`、`description` 必填)。本机已装 `unit-convert`、`json-tool`(见 `~/.pi/agent/skills/`)。启用 `/skill:` 命令:

```jsonc
// ~/.pi/agent/settings.json
{ "enableSkillCommands": true }
```

### 4.3 自定义 Provider(meraai 示例,见 §0 步骤 5)

- `~/.pi/agent/models.json`:`api` 字段定协议系列。
- `~/.pi/agent/auth.json`:API key,0600。
- 鉴权解析顺序:CLI `--api-key` → `auth.json` → 环境变量 → `models.json` 的 `apiKey`。

### 4.4 自主/长程能力(可选,对应 oh-my-codex 类)

本机已装(见 `PI_CAPABILITY.md` / `PI_GUIDE.md` §7):

| 能力 | 命令 | 说明 |
|------|------|------|
| ReAct 计划模式 | `pi --plan -p "..."` | 官方 `plan-mode` 示例(symlink 到扩展目录):只读 + 编号计划 |
| 并行子代理 | 对话里说 `Run 2 scouts in parallel ...` | 官方 `subagent` 示例 + `~/.pi/agent/agents/{scout,planner,reviewer,worker}.md` |
| 链式工作流 | `/implement ...` | prompts 已装 `~/.pi/agent/prompts/` |
| 长程自动循环 | `node /home/user/pi_test/tools/pi-autorun.js "任务" N` | 自研 RPC harness,最多 N 轮,`[TASK_COMPLETE]` 提前结束 |

安装方式(npm 包内示例,symlink):`plan-mode`、`subagent` 均来自
`/usr/local/node-v22.22.2-linux-x64-glibc-217/lib/node_modules/@earendil-works/pi-coding-agent/examples/extensions/`。
agents 去掉了 claude 模型引用,统一继承父会话 meraai 模型。

### 4.5 跨机快速开始(拷贝项目目录后)

项目目录自包含:`config/`(settings.json / models.json / extensions / agents / prompts / skills,**全实体文件,无 symlink**)、`.mcp.json`(mini/searxng 已用**相对路径**,跨机可用)、`mcp-servers/`、`tools/`。`auth.json` 不在项目里(密钥)。

新机器 4 步:

```bash
# ① 按 §0 装 pi(先设 Aliyun 源 + 本机 node 路径)
# ② 把项目 config/ 快照应用为运行时配置
mkdir -p ~/.pi/agent && cp -rL config/settings.json config/models.json config/extensions config/agents config/prompts config/skills ~/.pi/agent/
# ③ 写密钥(项目里没有;0600)
cat > ~/.pi/agent/auth.json <<'EOF'
{ "meraai": { "type": "api_key", "key": "sk-YOUR_KEY" } }
EOF
chmod 600 ~/.pi/agent/auth.json
# ④ 修 .mcp.json 机器相关路径:filesystem 的 npx 全路径是本机 WSL 值(WSL 有 Windows npx 污染必须全路径,见 P1);普通 Linux 换成 "npx" 即可。mini/searxng 相对路径不用改;SearXNG 地址按新机 SEARXNG_URL 改。
```

## 5. YOLO 模式:默认最高权限,只拦敏感路径

### 5.1 原理(重要,别找不存在的 flag)

Pi 0.84.3 **没有 `--yolo` 标志、没有 `skip-permissions` 开关、也没有内置的工具确认弹窗**。工具(`read`/`bash`/`edit`/`write`/`grep`/`find`/`ls`)默认就以进程权限**直接执行**,无沙箱。唯一会"询问"的是**项目信任**(默认 `ask`,只影响是否加载项目里的 `.pi/settings.json` 等资源)。

所以"yolo"在 Pi 上 = **关掉项目信任询问** + **用扩展给敏感路径加门禁**。

### 5.2 开启(本机已配置)

```jsonc
// ~/.pi/agent/settings.json
{
  "defaultProjectTrust": "always",   // ask | always | never → 永远自动信任项目,不再询问
  "defaultProvider": "meraai",       // 让 `pi` 免参数直跑
  "defaultModel": "deepseek-v4-flash"
}
```

- 单次:`pi --approve -p "..."`(`-a`)
- 交互内:`/trust` 保存信任决定(`~/.pi/agent/trust.json`)
- 非交互(`-p`/`--mode json`/`--mode rpc`)本来就不弹信任框

### 5.3 敏感路径守卫(skip-permission 的例外)

文件:`~/.pi/agent/extensions/sensitive-guard.ts`(全局扩展自动发现,重启生效)。

- 默认 yolo 全放行;仅拦截对敏感路径的 **读取/写入/编辑**(read/write/edit/grep/find/ls 的 `path`,以及 bash 命令里出现的路径变体 `~`/`$HOME`/`${HOME}`/绝对路径)。
- 拦截清单:`~/.ssh`、`~/.aws`、`~/.config/gcloud`、`~/.kube`、`~/.gnupg`、`~/.netrc`、`~/.git-credentials`、`~/.npmrc`、`~/.claude`、`~/.pi/agent/auth.json`、`/etc/shadow`、`/etc/gshadow`、`/etc/sudoers`、`/etc/sudoers.d`、`/etc/passwd`。
- 改清单/加 `.env` 拦截:直接编辑文件顶部 `SENSITIVE` 数组。
- 临时关守卫:`PI_GUARD_OFF=1 pi ...`。
- 拦截返回 `{ block: true, reason }`,模型会看到原因并据此调整。
- 实测:`read ~/.npmrc` → 拦截并返回原因;正常读项目文件不受影响。

> 注意:Pi 的"信任"只控制加载项目资源,**不是**工具执行权限沙箱。要隔离不可信代码请用容器(见 `docs/containerization.md`),不要把 `defaultProjectTrust: always` 当沙箱。

---

## 6. 关键问题与应对(harness)

> 每行都是本机实测过的坑 + 现成解法。AI agent 照着做即可,不必重新踩。

| # | 关键问题 | 应对(harness) |
|---|----------|----------------|
| P1 | **WSL 下 npx 被 Windows 版抢占**,MCP spawn 时报 `ERR_INVALID_URL` | MCP 配置 `command` 写 Linux npx 全路径 `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx`;别用 `which npx` 的结果 |
| P2 | **Zod v3/v4 冲突**:Pi 内置 Zod v4,MCP SDK 1.x 用 Zod v3,装某些适配器会报 `v3Schema.safeParse is not a function`,工具不出现 | 用 `@cansiny0320/pi-mcp-adapter@2.7.2`(显式支持 `^3.25.0 \|\| ^4.0.0`);装包后 `pi list` 确认在列,必要时 `/reload` |
| P3 | **GitHub 不可达**(git ls-remote / curl github.com 均超时),clone 不了 anthropics/skills、badlogic/pi-skills | npm 包走 Aliyun 源没问题;skills 暂时用本地自建(`~/.pi/agent/skills/`);网络恢复后再 clone 到 `~/.agents/skills/` 或在 settings.json `skills` 数组加路径 |
| P4 | **Mera AI 的 response 系列没实现**(`/v1/responses` → HTTP 500 not implemented);message ✅、chat/completions ✅ | provider 的 `api` 用 `anthropic-messages`(message 系列)或 `openai-completions`,别用 `openai-responses` |
| P5 | **`/mcp` 命令的 status/panel 是 TUI 功能**,headless(RPC / `-p`)下返回空 | headless 下用 `mcp({ })` 工具查状态/工具清单 |
| P6 | **RPC 模式 `printf ... \| pi --mode rpc` 不出结果**:stdin EOF 导致 pi 直接退出 | 客户端进程必须**保持 stdin 打开**(如 Node `child_process.spawn`),用 `readline` 逐行读 stdout JSONL |
| P7 | **RPC 连续快速发两条 prompt 报错**:agent 还在流式输出,第二条插不进 | 第二条用 `streamingBehavior: "followUp"`(排队等本轮完成)或先 `abort`;监控 `agent_settled` 事件确认空闲 |
| P8 | **auth.json 权限过宽**,token 可能泄露 | `chmod 600 ~/.pi/agent/auth.json`;token 别写进普通文件/git |
| P9 | **npm install 慢/超时**(国内) | `npm config set registry https://registry.npmmirror.com`(Aliyun);`npm ping` 验证 |
| P10 | **装包后功能不生效** | Pi 在启动时加载 packages;改完 settings/装完包要重启或 `/reload` |
| P11 | **模型在 `/model` 列表里灰着/连不上** | 三选一配好鉴权:`--api-key` / `auth.json` / 环境变量;用 `pi auth` 查凭据状态 |

---

## 7. 注意事项(安全与习惯)

1. **Extensions / Packages / MCP server 以完整系统权限运行**。安装前审阅源码;只连可信 MCP 源。对外可参照 §6 P2 选适配器、P3 控来源。
2. **Skills 能指示模型执行任意命令**(含执行脚本),启用前读一遍 `SKILL.md`。
3. **不要在文档/代码里写死 token**;用 `$VAR` 环境变量或 `~/.pi/agent/auth.json`(0600)。
4. Pi 读 `~/.pi/agent/AGENTS.md`(全局)与项目 `AGENTS.md`/`CLAUDE.md` 作为上下文,改后 `/reload`。
5. **Windows npx 污染**:凡是机器上有 Windows Node(WSL 常见),MCP/脚本 spawn 一律用 Linux 绝对路径(见 P1)。

---

## 8. 冒烟验证(装完必跑)

```bash
# ① 环境与版本
npm config get registry        # → https://registry.npmmirror.com
pi --version                   # → 0.84.3

# ② 基础问答(meraai)
pi --provider meraai --model deepseek-v4-flash -p "Reply with exactly: ok"   # → ok

# ③ MCP 端到端(filesystem)
pi --provider meraai --model deepseek-v4-flash -p "用 mcp 工具调用 filesystem 读取 /tmp/pi_mcp_test/hello.txt 内容并汇报"

# ④ Skills
pi --provider meraai --model deepseek-v4-flash -p "/skill:unit-convert 0 c f"   # → 0°C = 32°F

# ⑤ RPC(外部对接,另开终端)
pi --mode rpc --provider meraai --model deepseek-v4-flash
#   客户端保持 stdin 打开,发 {"type":"prompt","message":"..."} JSONL
```

全过 = 安装成功。失败先查 §6 对应行。

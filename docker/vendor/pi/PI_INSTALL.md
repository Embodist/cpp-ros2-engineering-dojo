# Pi 安装指南(使用 Aliyun Node 源)

> 本文档记录从零安装 Pi coding agent 及其核心插件(MCP 适配器、Skills)的完整步骤。
> 使用 **Aliyun 的 Node 源**(npm 镜像 `registry.npmmirror.com`)安装所有依赖。
> 配套文档:
> - 基础用法见 `PI_USAGE.md`
> - 进阶/MCP/Skills/外部对接见 `PI_GUIDE.md`
> - **给 AI agent 照着装的速查版见 `PI_AGENT_INSTALL.md`**(含关键问题与应对,机器人优先读它)
> - **本机能力清单见 `PI_CAPABILITY.md`**、**给 AI 的一键安装 prompt 见 `PI_INSTALL_PROMPT.md`**

---

## 1. 环境与版本(本机已装)

| 组件 | 版本 | 位置 |
|------|------|------|
| Node.js | v22.22.2 | `/usr/local/node-v22.22.2-linux-x64-glibc-217/` |
| npm | 10.9.7 | 随 node 同目录 |
| pi (pi-coding-agent) | 0.84.3 | 全局包 `@earendil-works/pi-coding-agent` |
| pi 可执行文件 | — | `/usr/bin/pi` → `/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/pi` |

### 1.1 配置 npm 使用 Aliyun 源

`registry.npmmirror.com` 是阿里云维护的官方 npm 镜像(原 taobao 镜像)。

```bash
# 写入用户级配置 ~/.npmrc
npm config set registry https://registry.npmmirror.com

# 验证
npm config get registry            # → https://registry.npmmirror.com
npm ping                            # → PONG
```

`~/.npmrc` 内容:

```
registry=https://registry.npmmirror.com
```

> **Node 二进制镜像(可选)**:若需从国内安装 Node 本身,可用
> `https://npmmirror.com/mirrors/node/`(或阿里云 `https://mirrors.aliyun.com/nodejs-release/`)。
> 本机 node 已装好,无需此步。

---

## 2. 安装 pi-agent

```bash
npm install -g --ignore-scripts @earendil-works/pi-coding-agent
```

- `--ignore-scripts`:跳过依赖的生命周期脚本,官方推荐。
- 安装目录(取决于 npm prefix,本机为 `/usr/local/node-v22.22.2-linux-x64-glibc-217`):
  - 包本体:`lib/node_modules/@earendil-works/pi-coding-agent/`
  - 可执行:`bin/pi`
- 如需固定版本:`npm install -g @earendil-works/pi-coding-agent@0.84.3`。

### 2.1 验证安装

```bash
pi --version          # → 0.84.3
pi --help             # → 选项与命令帮助
pi --list-models      # → 列出内置 provider 模型(默认 google/anthropic 等)
pi                    # → 进入交互模式(有 TTY 时)
```

---

## 3. 初始化配置目录

首次运行时 Pi 自动创建 `~/.pi/agent/`。手工初始化:

```bash
mkdir -p ~/.pi/agent/skills ~/.pi/agent/extensions ~/.pi/agent/sessions
chmod 700 ~/.pi/agent
```

核心文件(状态与配置总览):

| 文件/目录 | 作用 |
|-----------|------|
| `~/.pi/agent/settings.json` | 全局设置(`theme`、`packages`、`enableSkillCommands`、`defaultProjectTrust`、`defaultProvider`、`defaultModel`) |
| `~/.pi/agent/auth.json` | 凭据(API key / OAuth token,权限 0600) |
| `~/.pi/agent/models.json` | 自定义 provider / 模型(声明式) |
| `~/.pi/agent/models-store.json` | 拉取的模型目录缓存 |
| `~/.pi/agent/extensions/` | 全局扩展自动发现目录(`*.ts`,`/reload` 热重载)。本机已装 `sensitive-guard.ts`(YOLO 敏感路径守卫,见 §5.3) |
| `~/.pi/agent/skills/` | 全局 skills 目录(`SKILL.md`) |
| `~/.pi/agent/sessions/` | 会话文件(JSONL) |
| `~/.pi/agent/trust.json` | 项目信任决定(`/trust` 写入) |
| `~/.pi/agent/mcp.json` | 全局 MCP 配置(adapter 用;项目级优先用 `.mcp.json`) |
| 项目 `.pi/` | 项目级设置/扩展/skills(需信任) |

---

## 4. 安装核心插件(Packages / MCP / Skills)

### 4.1 MCP 适配器(核心插件)

Pi 无内置 MCP,通过扩展包接入。本机安装的是社区适配器:

```bash
pi install npm:@cansiny0320/pi-mcp-adapter
```

- 写入 `~/.pi/agent/settings.json` 的 `packages` 数组;卸载用 `pi remove npm:@cansiny0320/pi-mcp-adapter`。
- 临时试用不安装:`pi -e npm:@cansiny0320/pi-mcp-adapter`。
- 配置 MCP server:项目 `.mcp.json`(见 `PI_GUIDE.md` §4.1),本机已配置 `filesystem` + `mini` 两个 server。
- **WSL 注意**:MCP 配置里 `npx` 必须写 Linux 全路径(`/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx`),否则会命中 Windows npx 报 `ERR_INVALID_URL`。

其他可选的 MCP 扩展:`npm:pi-mcp-extension`、`git:github.com/tickernelz/pi-mcp-tools`(本机 GitHub 不可达,未装)。

### 4.2 Skills

Pi 遵循 Agent Skills 标准,`SKILL.md` + frontmatter(`name`/`description`)。

```bash
mkdir -p ~/.pi/agent/skills/<skill-name>/scripts
```

本机已装:

| Skill | 说明 | 脚本 |
|-------|------|------|
| `unit-convert` | 单位换算(摄氏/华氏、km/mi、kg/lb) | `scripts/convert.sh` |
| `json-tool` | JSON 校验 / 美化 / 按路径取值 | `scripts/json-tool.js` |

启用 skill 命令(在 `~/.pi/agent/settings.json`):

```json
{ "enableSkillCommands": true }
```

> GitHub 恢复后可 clone 官方 skills 仓库使用:
> `git clone https://github.com/anthropics/skills ~/.agents/skills`
> 或 `git clone https://github.com/badlogic/pi-skills ~/.agents/skills`。

### 4.3 自定义 Provider(可选,meraai 示例)

```bash
# 1. 写 models.json 注册 provider(见 PI_USAGE.md §3.2)
# 2. 把 API key 写入 auth.json(权限 0600)
cat > ~/.pi/agent/auth.json <<'EOF'
{ "meraai": { "type": "api_key", "key": "sk-..." } }
EOF
chmod 600 ~/.pi/agent/auth.json
```

---

## 5. 从零到可用的验证清单

```bash
# ① 环境
npm config get registry                                    # npmmirror(Aliyun)
node --version && pi --version

# ② 基础问答
pi --provider meraai --model deepseek-v4-flash -p "Reply with exactly the word: hello"   # → hello

# ③ MCP
pi --list-models                                           # 确认包加载无报错
# 用 mcp() 工具:状态 mcp({}) / 直调 mcp({ tool, args })

# ④ Skills
pi --provider meraai --model deepseek-v4-flash -p "/skill:unit-convert 0 c f"           # → 0°C = 32°F

# ⑤ 外部对接(RPC)
pi --mode rpc --provider meraai --model deepseek-v4-flash   # 保持 stdin 打开,逐行发 JSONL 命令
```

---

## 6. 常用维护命令

```bash
pi list              # 列出已装包
pi update            # 更新 pi 本身
pi update --all      # 更新 pi + 包 + 对账 git refs
pi update --models   # 仅刷新模型目录
pi remove npm:@cansiny0320/pi-mcp-adapter   # 卸载包
pi config            # TUI 启用/禁用包内资源(Tab 切全局/项目)
npm uninstall -g @earendil-works/pi-coding-agent   # 卸载 pi(保留 ~/.pi/agent 数据)
```

---

## 7. 排错速查

| 现象 | 处理 |
|------|------|
| `npm install` 慢/失败 | 确认 `registry` 为 `https://registry.npmmirror.com` |
| `pi` 命令找不到 | 检查 npm prefix 下 `bin/pi` 是否存在,并确认 `PATH` 含该 bin 目录 |
| MCP 服务器 `ERR_INVALID_URL` | WSL 下 npx 被 Windows 版抢占,改 Linux npx 全路径 |
| 模型在 `/model` 灰着 | auth.json / 环境变量 / `--api-key` 三选一配好鉴权 |
| 装包后功能不生效 | 重启 pi 或 `/reload`;检查 `pi list` 包在列 |

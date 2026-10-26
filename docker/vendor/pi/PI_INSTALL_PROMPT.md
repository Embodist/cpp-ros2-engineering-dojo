# Pi 一键安装 Prompt(给 AI Agent 用)

> **用法**:把下面 ```prompt 代码块里的内容**原样复制**给 Claude Code / Codex / OpenCode 等 AI agent,它会自动:
> 1. 读 `/home/user/pi_test/` 下的安装文档与能力清单;
> 2. 对照本机能力清单逐项**检查→补齐**缺失的插件/配置;
> 3. 跑冒烟验证并汇报。
>
> 已装好的机器跑它 = 体检 + 修复;全新机器跑它 = 从零装好。本机当前状态见 `PI_CAPABILITY.md`。

```prompt
你是 Pi coding agent 的安装/体检助手。目标:让本机 Pi 达到"可直接使用"的完整状态——
包括 pi-agent、MCP 适配器、MCP servers、skills、自定义 provider(meraai)、YOLO 敏感路径守卫,
并且全部通过冒烟测试。参考文档与目标状态都在 /home/user/pi_test/。

【第一步:读文档】
依次完整读取以下文件,以它们为准:
- /home/user/pi_test/PI_CAPABILITY.md   (本机能力清单 = 目标状态)
- /home/user/pi_test/PI_AGENT_INSTALL.md(安装速查,含 TL;DR、关键问题与应对、YOLO 模式)

【本机环境事实(不要再探测,直接用)】
- OS:WSL2;Node v22.22.2 at /usr/local/node-v22.22.2-linux-x64-glibc-217
- npm registry 必须是 https://registry.npmmirror.com(Aliyun);缺就 npm config set registry https://registry.npmmirror.com
- pi 可执行:/usr/bin/pi(装后若无则用 npm install -g --ignore-scripts @earendil-works/pi-coding-agent)
- MCP 配置里 npx 一律用 Linux 全路径:/usr/local/node-v22.22.2-linux-x64-glibc-217/bin/npx
  (绝不能用 Windows npx:/mnt/d/... ,会 ERR_INVALID_URL)
- provider:meraai,baseUrl https://api.meraai.cn,api 必须为 anthropic-messages(response 系列该网关未实现)

【逐项检查,缺哪个装哪个;已存在的不要动】
1. pi 已装且版本≥0.84:pi --version
2. 包:MCP 适配器 @cansiny0320/pi-mcp-adapter——pi list 无则 pi install npm:@cansiny0320/pi-mcp-adapter
3. 全局设置 ~/.pi/agent/settings.json 需包含(缺的补,已有键别覆盖):
   defaultProjectTrust=always、enableSkillCommands=true、defaultProvider=meraai、defaultModel=deepseek-v4-flash、
   packages=["npm:@cansiny0320/pi-mcp-adapter"]
4. 项目 MCP 配置 /home/user/pi_test/.mcp.json:filesystem(命令=Linux npx 全路径,args 带 -y @modelcontextprotocol/server-filesystem@latest <dir>)+ mini(命令=node,args 本地 mini-server.js)。缺 server 文件就创建(参考 mini 协议:stdio JSON-RPC 换行分隔)
5. 自定义 provider ~/.pi/agent/models.json:providers.meraai(api=anthropic-messages,baseUrl=api.meraai.cn,models 含 deepseek-v4-flash 等 10 个)
6. 鉴权 ~/.pi/agent/auth.json:meraai 的 api_key。存在则保持,缺失要用户提供 key;完成后 chmod 600
7. YOLO 敏感守卫 ~/.pi/agent/extensions/sensitive-guard.ts:不存在就按 PI_AGENT_INSTALL.md §5.3 建(拦截 ~/.ssh ~/.aws ~/.config/gcloud ~/.kube ~/.gnupg ~/.netrc ~/.git-credentials ~/.npmrc ~/.claude ~/.pi/agent/auth.json /etc/shadow /etc/gshadow /etc/sudoers /etc/sudoers.d /etc/passwd 的读写,支持 PI_GUARD_OFF=1 开关)
8. skills:unit-convert、json-tool 在 ~/.pi/agent/skills/(缺则建,含 SKILL.md + scripts/)

【安全与注意事项(务必遵守)】
- auth.json 权限必须 0600;不要把 token 写进文档/代码/提交 git
- 扩展/Packages/MCP server 都是完整系统权限,只装可信来源;不执行任何 rm -rf、不 sudo 改系统文件
- 不修改 meraai 的 API key、不改 ~/.pi/agent/auth.json 内容(除非用户明确给出新 key)
- 不删除用户已有配置/文件;只补缺、只修坏
- Windows npx 污染:凡 spawn npx 一律用 Linux 全路径
- Zod v3/v4 冲突:适配器必须用 @cansiny0320/pi-mcp-adapter(支持 Zod v4);装后 pi list 确认在列
- GitHub 不可达时不要尝试 git clone;skills 用本地自建即可

【验收:全部跑通才算完成】
- pi --version
- npm config get registry → npmmirror
- pi list → 含 @cansiny0320/pi-mcp-adapter
- pi -p "Reply with exactly: ok" → ok(默认 meraai)
- pi -p "/skill:unit-convert 0 c f" → 0°C = 32°F
- pi -p "用 read 工具读取 ~/.npmrc" → 应被敏感守卫拦截并返回原因
- pi --mode rpc 启动正常(保持 stdin 打开,发一条 prompt 能看到事件流)

【汇报】
最后用表格汇报:每项"已存在/已修复/新装",并给出冒烟测试的通过情况与任何遗留问题。
不要中途询问,除非:缺少 auth key / 遇到无法自主解决的环境问题。
```

> **给用户的小抄**:交给 AI 前可加一行后缀 `只检查与安装,不要聊天;结束后贴验收结果给我`。若要临时放行敏感目录,告诉 agent `PI_GUARD_OFF=1` 或让它改 `sensitive-guard.ts` 里的 `SENSITIVE` 数组。

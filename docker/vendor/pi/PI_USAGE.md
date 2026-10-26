# Pi 使用文档(接入 Mera AI 自定义 Provider)

本文档基于 `@earendil-works/pi-coding-agent` v0.84.3,记录了如何在当前环境启动 Pi、如何使用自定义 provider(Mera AI `api.meraai.cn`)以及如何扩展。官方文档参考:官方 `docs/providers.md`、`docs/models.md`、`docs/custom-provider.md`。

## 1. Pi 是什么

Pi 是一个 AI 编程助手 CLI(读文件 / 编辑 / 写文件 / 执行 bash 四大工具),由 `@earendil-works/pi-coding-agent` 提供。它通过 **Provider** 概念对接各家模型后端。

本机安装:

```bash
pi --version        # 0.84.3
pi --help           # 全部 CLI 选项
```

## 2. 快速启动

```bash
# 交互模式(在项目目录下)
cd /home/user/pi_test
pi

# 非交互一次性问答
pi -p "总结这个仓库"

# 指定 provider / 模型
pi --provider meraai --model deepseek-v4-flash

# 恢复上次会话 / 浏览历史会话
pi -c
pi -r
```

启动时 Pi 会加载上下文文件:`~/.pi/agent/AGENTS.md`(全局)、项目目录下的 `AGENTS.md` / `CLAUDE.md`。修改后需 `/reload` 或重启。

## 3. 自定义 Provider:Mera AI(api.meraai.cn)

### 3.1 它属于哪个系列(重点)

`api` 字段决定 Pi 使用哪套流式协议,**三个系列**:

| api 值 | 系列 | 说明 |
|--------|------|------|
| `anthropic-messages` | **message 系列** | Anthropic Messages API 及其兼容后端 |
| `openai-completions` | **chat_completion 系列** | OpenAI Chat Completions API 及其兼容后端 |
| `openai-responses` | **response 系列** | OpenAI Responses API |

**对 `api.meraai.cn` 的实测结果**(2026-08-26 用本机 token curl 探测):

- `POST /v1/messages` → **HTTP 200,可用**(返回 thinking 块 + signature,标准 Anthropic thinking 格式)→ **message 系列**
- `POST /v1/chat/completions` → **HTTP 200,可用**(返回 `reasoning_content`,deepseek 风格思考)→ **chat_completion 系列**
- `POST /v1/responses` → **HTTP 500 `not implemented`** → **response 系列不可用**

**结论:本配置选择 `api: "anthropic-messages"`(message 系列)**。原因:

1. 与当前 Claude Code 对同一后端的用法完全一致(`ANTHROPIC_BASE_URL=https://api.meraai.cn`),同一 token 即可。
2. Pi 对 anthropic-messages 的 thinking 块、工具调用、流式支持最完整。
3. 若你偏好,也可改用 `openai-completions`,但不要用 `openai-responses`(后端未实现)。

### 3.2 配置文件

Pi 的 provider 配置在 `~/.pi/agent/models.json`(declarative,无需写代码;`/model` 界面每次打开都会重新加载):

```jsonc
{
  "providers": {
    "meraai": {
      "name": "Mera AI",
      "baseUrl": "https://api.meraai.cn",
      "api": "anthropic-messages",      // message 系列
      "apiKey": "$MERAAI_API_KEY",      // 兜底:环境变量;实际 key 存在 auth.json
      "models": [
        { "id": "deepseek-v4-flash",    "name": "DeepSeek V4 Flash",    "reasoning": true, "input": ["text"],                "contextWindow": 131072, "maxTokens": 16384 },
        { "id": "deepseek-v4-pro",      "name": "DeepSeek V4 Pro",      "reasoning": true, "input": ["text"],                "contextWindow": 131072, "maxTokens": 16384 },
        { "id": "deepseek-v4-flash-vision-exp", "name": "DeepSeek V4 Flash Vision", "reasoning": true, "input": ["text", "image"], "contextWindow": 131072, "maxTokens": 16384 },
        { "id": "glm-5.2",              "name": "GLM-5.2",              "reasoning": true, "input": ["text"],                "contextWindow": 262144, "maxTokens": 16384 },
        { "id": "glm-5.3",              "name": "GLM-5.3",              "reasoning": true, "input": ["text"],                "contextWindow": 262144, "maxTokens": 16384 },
        { "id": "kimi-k2.7-code",       "name": "Kimi K2.7 Code",       "reasoning": true, "input": ["text"],                "contextWindow": 262144, "maxTokens": 16384 },
        { "id": "MiniMax-M2.7",         "name": "MiniMax M2.7",         "reasoning": true, "input": ["text"],                "contextWindow": 200000, "maxTokens": 16384 },
        { "id": "MiniMax-M2.7-highspeed","name": "MiniMax M2.7 Highspeed","reasoning": true,"input": ["text"],                "contextWindow": 200000, "maxTokens": 16384 },
        { "id": "qwen3.6-flash",        "name": "Qwen 3.6 Flash",       "reasoning": true, "input": ["text"],                "contextWindow": 262144, "maxTokens": 16384 },
        { "id": "qwen3.7-max",          "name": "Qwen 3.7 Max",         "reasoning": true, "input": ["text"],                "contextWindow": 262144, "maxTokens": 16384 }
      ]
    }
  }
}
```

- `baseUrl` 填根地址即可,Pi 会自动拼 `/v1/messages`。
- 模型清单取自 `GET https://api.meraai.cn/v1/models`,上面是筛选出的编程向模型。可随时增删。
- `cost` 未知,当前全为 0;如后端有计费价目,在对应模型里补上 `cost: { input, output, cacheRead, cacheWrite }`($/百万 token)。
- `contextWindow` / `maxTokens` 是估算值,可按实际调整。

### 3.3 鉴权

Pi 的凭据解析顺序(参考 `providers.md` Resolution Order):

1. CLI `--api-key`
2. `~/.pi/agent/auth.json` 条目
3. 环境变量
4. `models.json` 中的 `apiKey`

本机已经把 Mera AI 的 key 写入了 `~/.pi/agent/auth.json`(权限 0600):

```json
{
  "meraai": { "type": "api_key", "key": "sk-..." }
}
```

`models.json` 里的 `"apiKey": "$MERAAI_API_KEY"` 是环境变量兜底,`auth.json` 优先级更高。两者都满足时用 auth.json。

也可用交互命令管理凭据:

```text
/login        # 交互选择 provider / 输入 API key
/logout       # 清除凭据
```

### 3.4 使用示例

```bash
# 列出 meraai 可用模型
pi --list-models meraai

# 指定模型跑一次性问答
pi --provider meraai --model deepseek-v4-flash -p "你好"

# 指定思考等级(如果模型支持)
pi --provider meraai --model deepseek-v4-pro --thinking high -p "..."
```

交互模式下:

- `/model` 或 `Ctrl+L` 切换模型(会在 meraai 模型间切换)
- `Shift+Tab` 循环思考等级;`Ctrl+P` / `Shift+Ctrl+P` 循环 scoped 模型
- `!命令` 执行 shell 并把输出喂给模型;`!!命令` 只执行不喂上下文
- `/new` 开新会话,`/resume` / `/fork` / `/tree` 管理会话

## 4. 官方 providers.md 速查(其他后端)

Pi 内置了大量订阅/API-key provider。如需接入别的后端,可按官方表对照环境变量名:

| 后端 | 环境变量 | `auth.json` key |
|------|----------|------------------|
| Anthropic | `ANTHROPIC_API_KEY` | `anthropic` |
| OpenAI | `OPENAI_API_KEY` | `openai` |
| DeepSeek | `DEEPSEEK_API_KEY` | `deepseek` |
| Google Gemini | `GEMINI_API_KEY` | `google` |
| xAI | `XAI_API_KEY` | `xai` |
| OpenRouter | `OPENROUTER_API_KEY` | `openrouter` |
| 阿里百炼 | `ZAI_CODING_CN_API_KEY`(示例) | `zai-coding-cn` |
| Ollama(本地) | — | 见 models.json 示例 |

接入方式两种:

- **`models.json`(零代码)**:任意支持 `openai-completions` / `openai-responses` / `anthropic-messages` / `google-generative-ai` 的端点,填 `baseUrl` + `api` + `models` 即可。本仓库即此方式。
- **Extension(需代码)**:自定义流式实现、OAuth/SSO 时用 `pi.registerProvider()`,参考官方 `docs/custom-provider.md` 和 `examples/extensions/custom-provider-gitlab-duo/`。

## 5. 排错

- **模型在后端不存在**:报 404,核对 `GET /v1/models` 返回的 `id`。
- **选了 `openai-responses`**:Mera AI 后端未实现,报 `not implemented`,改用 `anthropic-messages` 或 `openai-completions`。
- **模型在 `/model` 里灰着不可选**:没配好 auth(auth.json / env / --api-key 三选一)。
- **上下文超限**:Pi 会自动压缩并重试;若后端报错文案 Pi 不识别,需在扩展里用 `message_end` 把 `errorMessage` 归一化到 `context_length_exceeded`(见官方 custom-provider.md)。
- **某些非 deepseek 模型(glm/kimi/minimax/qwen)在 anthropic 协议下行为异常**:优先使用 deepseek-v4-flash / pro;如确实有问题,可为该模型单独设 `api: "openai-completions"`(模型级覆盖),并用 `compat.thinkingFormat` 微调思考参数。

## 6. 已完成的验证

```bash
pi --provider meraai --model deepseek-v4-flash -p "Reply with exactly the word: hello"   # → hello
pi --provider meraai --model deepseek-v4-flash -p "用 bash 列出当前目录文件并报告数量"        # → 正确调用 bash 工具并汇总
```

两条均通过,聊天与工具调用链路均可用。

# Vendored 配置快照

这里是非敏感的 Neovim / Pi 配置快照，供 **CI 和公开构建** 使用，避免依赖维护者本机的 dotfiles 仓库。

| 路径 | 来源 |
| :--- | :--- |
| `nvim/` | `~/neovim.dotfiles/nvim`（init.lua、lua、snippets、lazy-lock.json 等） |
| `pi/config/` | `~/dotfiles.pi/config` 的 settings/models/extensions/agents/prompts/skills |
| `pi/mcp-servers/` | `~/dotfiles.pi/mcp-servers` |
| `pi/tools/` | `~/dotfiles.pi/tools` 的 package.json / lock / autorun 脚本 |
| `pi/PI_*.md` | `~/dotfiles.pi` 的文档 |

## 刷新

```bash
./docker/sync-vendor.sh
```

## 安全边界

- **不包含** `auth.json`、`.env`、`.git`、`node_modules`、sessions。
- `sync-vendor.sh` 会在复制后扫描疑似真实 key；发现即失败。
- 镜像运行时密钥只通过环境变量或用户挂载注入。

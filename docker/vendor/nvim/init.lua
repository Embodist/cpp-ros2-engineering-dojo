-- ============================================================================
--  neovim.dotfiles  ·  C++ / CAE-EDA / 竞赛 / 工程化 专用配置
--  入口：引导 lazy.nvim + LazyVim + lua/plugins/*
--
--  目录约定（本仓库）：
--    nvim/                 -> ~/.config/nvim   （符号链接，见 install.sh）
--    nvim/lua/config/      -> 全局选项 / 键位 / autocmd / lazy 引导
--    nvim/lua/plugins/     -> 按主题拆分的插件配置（cpp / competitive / ai ...）
--    nvim/snippets/        -> LuaSnip 自定义代码片段（cpp.json）
--
--  文档：docs/guide/   ·  交接：docs/session-handoff.md  ·  变更：docs/changelog.md
-- ============================================================================

require("config.lazy")

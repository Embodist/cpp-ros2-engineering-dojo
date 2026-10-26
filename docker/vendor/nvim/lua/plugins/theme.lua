-- ============================================================================
--  主题插件与主题选择器接线
--    · kanagawa     —— 本仓库新增的主题（想再加主题：在此加插件，再到
--                      nvim/lua/config/theme.lua 的 M.themes 里登记名字）
--    · tokyonight / catppuccin —— LazyVim 自带（见 LazyVim colorscheme.lua），
--                      需要改它们的默认风格时在下面 override opts 即可
--    · LazyVim/LazyVim —— 把默认配色换成我们的“记住上次选择”加载器
--
--  用法：<leader>uM 下一个 / <leader>um 上一个 / <leader>uC 选择器（实时预览）
--  文档：docs/guide/22-theme.md
-- ============================================================================

return {
  -- ── 新增主题：kanagawa ───────────────────────────────────────────────────
  --    可选 theme: "wave"(默认) | "dragon" | "lotus"
  {
    "rebelot/kanagawa.nvim",
    lazy = true,
    opts = {
      theme = "wave",
      background = { dark = "wave", light = "lotus" },
      compile = false,
    },
  },

  -- ── 想换 tokyonight 的风格就在这里覆盖（LazyVim 默认 style = "moon"）─────
  -- {
  --   "folke/tokyonight.nvim",
  --   opts = { style = "night" },   -- night | storm | moon | day
  -- },

  -- ── 让 LazyVim 用我们的主题加载器（保证重启后仍是上次选的主题）───────────
  {
    "LazyVim/LazyVim",
    opts = {
      colorscheme = function()
        require("config.theme").load()
      end,
    },
  },
}

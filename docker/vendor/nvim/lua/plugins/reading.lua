-- ============================================================================
--  代码阅读（大型 C++ 代码库，如 MFEM / suanPan / nextpnr）
--    · glance.nvim   → 不跳转窗口，浮层预览定义/引用（读代码神器）
--    · aerial        → 符号大纲（LazyVim extra 提供，<leader>cs）
--    · navic         → 状态栏面包屑（LazyVim extra 提供）
--    · treesitter-context → 顶部固定显示当前函数上下文（<leader>ut 开关）
--
--  文档：docs/guide/10-code-reading.md
-- ============================================================================

return {
  {
    "dnlhc/glance.nvim",
    cmd = "Glance",
    keys = {
      { "gD", "<cmd>Glance definitions<cr>", desc = "Glance: 预览定义" },
      { "gR", "<cmd>Glance references<cr>", desc = "Glance: 预览引用" },
      { "gY", "<cmd>Glance type_definitions<cr>", desc = "Glance: 预览类型定义" },
      { "gM", "<cmd>Glance implementations<cr>", desc = "Glance: 预览实现" },
    },
    opts = {
      border = { enable = true },
      list = {
        width = 0.33,
        position = "right",
      },
      preview_win_opts = {
        cursorline = true,
        number = true,
      },
      folds = { folded = false, foldmethod = "manual" },
    },
  },
}

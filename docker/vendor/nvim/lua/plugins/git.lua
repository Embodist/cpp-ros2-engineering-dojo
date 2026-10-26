-- ============================================================================
--  Git 工作流
--    · gitsigns   → 行内 hunk / blame（LazyVim 内置，<leader>gh*）
--    · lazygit    → 终端 TUI（LazyVim 内置，需安装 lazygit 二进制，<leader>gg）
--    · diffview   → 结构化的 diff / 文件历史视图（本文件新增）
--    · octo       → GitHub PR / Issue（LazyVim util.octo extra，<leader>gi/gp）
--
--  文档：docs/guide/14-plugins-reference.md
-- ============================================================================

return {
  {
    "sindrets/diffview.nvim",
    cmd = {
      "DiffviewOpen",
      "DiffviewClose",
      "DiffviewToggleFiles",
      "DiffviewFocusFiles",
      "DiffviewRefresh",
      "DiffviewFileHistory",
    },
    keys = {
      { "<leader>gv", "<cmd>DiffviewOpen<cr>", desc = "Git: Diff 视图" },
      { "<leader>gV", "<cmd>DiffviewClose<cr>", desc = "Git: 关闭 Diff 视图" },
    },
    opts = {
      enhanced_diff_hl = true,
      view = {
        merge_tool = { layout = "diff3_mixed", disable_diagnostics = true },
      },
    },
  },
}

-- ============================================================================
--  UI / 交互微调
--  LazyVim 已提供：tokyonight 主题、lualine、bufferline、which-key、noice、
--  snacks（dashboard/explorer/picker/indent/terminal）、todo-comments、trouble 等。
--  这里只补充/覆盖少量与 C++ 长文件阅读相关的体验项。
-- ============================================================================

return {
  {
    "folke/snacks.nvim",
    opts = {
      -- 通知：统一走 snacks.notifier
      notifier = { enabled = true, timeout = 3000, style = "compact" },
      -- 终端：默认底部，C++ 编译/运行输出更易读
      terminal = {
        win = { position = "bottom", height = 0.35 },
      },
      -- 平滑滚动（大文件阅读）
      scroll = { enabled = true },
      -- 注意：words 高亮由 editor.illuminate extra 接管，这里不要重复开启
    },
  },

  {
    "folke/trouble.nvim",
    opts = {
      focus = true, -- 打开 trouble 时自动聚焦
      warn_no_results = false,
    },
  },

  -- which-key 分组名称（让 <leader>j / <leader>k 菜单更清晰）
  {
    "folke/which-key.nvim",
    opts = {
      spec = {
        { "<leader>j", group = "数据文件 (JSON/YAML/XML/TOML)" },
        { "<leader>k", group = "竞赛 / 测试用例" },
        { "<leader>m", group = "内存 / 性能 / 安全分析" },
        { "<leader>n", group = "C++ 代码生成 / 文档注释" },
        { "<leader>R", group = "ROS 2 / 机器人" },
      },
    },
  },
}

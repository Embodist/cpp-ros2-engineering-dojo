-- ============================================================================
--  效率增强（UX）——逐个功能唯一，不与既有插件重复
--    · rainbow-delimiters → 彩虹括号（深嵌套 C++ 模板可读性）
--    · nvim-bqf           → 更强大的 quickfix（构建错误定位）
--    · undotree           → 撤销历史树
--    · vim-tmux-navigator → Neovim 与 tmux 面板无缝 <C-hjkl> 切换
--    · nvim-spider        → 更聪明的 w/e/b（按子词、跳过标点）
--    · fidget.nvim        → LSP 索引进度（LazyVim v16 未内置）
--    · nvim-lightbulb     → 有代码动作时显示 💡（提示用 <leader>ca）
--
--  文档：docs/guide/17-efficient-plugins.md
-- ============================================================================

return {
  -- ── 彩虹括号 ────────────────────────────────────────────────────────────
  --    ⚠️ 不能用 opts = {}：lazy 会去调 require("rainbow-delimiters").setup()，
  --    而该插件的主模块没有 setup（会报 “Failed to run config for rainbow-delimiters.nvim:
  --    attempt to call field 'setup' (a nil value)”）。官方入口是 rainbow-delimiters.setup。
  {
    "HiPhish/rainbow-delimiters.nvim",
    event = "LazyFile",
    config = function()
      require("rainbow-delimiters.setup").setup({})
    end,
  },

  -- ── quickfix 增强（C++ 构建报错必备）────────────────────────────────────
  --    默认在 quickfix 内按 p 预览；支持 treesitter 高亮 + fzf
  {
    "kevinhwang91/nvim-bqf",
    ft = "qf",
    opts = {
      auto_enable = true,
      preview = {
        auto_preview = true,
        border = "rounded",
        show_title = true,
        win_height = 15,
        winblend = 0,
      },
      magic_window = true,
    },
  },

  -- ── 撤销历史树 ──────────────────────────────────────────────────────────
  --    C++ 大改动后回溯“我到底改了什么”非常好用
  {
    "mbbill/undotree",
    cmd = "UndotreeToggle",
    keys = {
      { "<leader>uu", "<cmd>UndotreeToggle<cr>", desc = "撤销历史树" },
    },
  },

  -- ── tmux 无缝导航 ───────────────────────────────────────────────────────
  --    本机使用 tmux（~/.tmux.conf.local）。插件端关闭默认映射，
  --    改由 config 在 VeryLazy（LazyVim 键位之后）接管 <C-hjkl>，确保优先生效。
  --    tmux 端也需配置，见 docs/guide/17-efficient-plugins.md#2-tmux-无缝导航
  {
    "christoomey/vim-tmux-navigator",
    lazy = false,
    init = function()
      vim.g.tmux_navigator_no_mappings = 1
    end,
    config = function()
      local map = vim.keymap.set
      vim.api.nvim_create_autocmd("User", {
        pattern = "VeryLazy",
        once = true,
        callback = function()
          map("n", "<C-h>", "<Plug>TmuxNavigateLeft", { desc = "Tmux/窗口 左" })
          map("n", "<C-j>", "<Plug>TmuxNavigateDown", { desc = "Tmux/窗口 下" })
          map("n", "<C-k>", "<Plug>TmuxNavigateUp", { desc = "Tmux/窗口 上" })
          map("n", "<C-l>", "<Plug>TmuxNavigateRight", { desc = "Tmux/窗口 右" })
        end,
      })
    end,
  },

  -- ── 更聪明的词移动（shorter/子词感知）───────────────────────────────────
  --    会覆盖默认的 w/e/b/ge/gE；不喜欢可加 opts = { skipInsignificantPunctuation = false }
  --    或设 vim.g.spider_disable_default_mappings = true 后自定义
  {
    "chrisgrieser/nvim-spider",
    event = "LazyFile",
    opts = {
      skipInsignificantPunctuation = true,
    },
  },

  -- ── LSP 进度提示 ────────────────────────────────────────────────────────
  --    clangd 后台索引时在右下角显示进度，避免“卡住”错觉
  {
    "j-hui/fidget.nvim",
    event = { "LspAttach", "BufReadPost" },
    opts = {
      progress = {
        suppress_on_insert = true,
        ignore_done_already = true,
      },
      notification = {
        window = { winblend = 0 },
      },
    },
  },

  -- ── 代码动作提示灯 ──────────────────────────────────────────────────────
  --    光标处有代码动作（clangd 快速修复/重构）时显示 💡，提示 <leader>ca
  {
    "kosayoda/nvim-lightbulb",
    event = { "LspAttach", "BufReadPost" },
    opts = {
      autocmd = { enabled = true },
      virtual_text = { enabled = true },
      priority = 4096,
    },
  },
}

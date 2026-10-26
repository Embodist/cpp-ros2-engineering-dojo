-- ============================================================================
--  Python / Node(JS·TS) 语言支持
--    · LSP：pyright + ruff（Python） / vtsls（JS·TS）—— 均来自 mason（npm / GitHub）
--    · 格式化：Python 用 ruff；JS·TS 由 formatting.prettier extra 负责
--    · 测试：neotest-python（需要 pytest）
--    · 调试：nvim-dap-python（需要 debugpy-adapter）
--    · Treesitter：python / typescript / tsx / javascript / ninja / rst
--
--  依赖 LazyVim extras：lang.python / lang.typescript（见 lazyvim.json）
--  文档：docs/guide/21-python-node.md
--
--  ⚠️ 本文件同时是 **pypi 类 mason 包的唯一过滤入口**（见文件末尾）：
--     mason 的 pypi provider 在本网络环境下会失败，失败时还会级联清除同批正在安装的
--     其他包（实测把 codelldb 目录删成悬空链接）。故统一剔除，改用 apt / pipx 提供。
-- ============================================================================

return {
  -- ── Treesitter 解析器补充（C/C++ 的在 cpp.lua）───────────────────────────
  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = {
        -- Python
        "python",
        "rst",
        "ninja",
        -- JS / TS
        "javascript",
        "typescript",
        "tsx",
        "jsdoc",
      },
    },
  },

  -- ── Python 格式化：ruff（mason 装的是 GitHub 官方二进制）────────────────
  --    <leader>cf / 保存时自动格式化（<leader>uf 开关）
  {
    "stevearc/conform.nvim",
    opts = {
      formatters_by_ft = {
        python = { "ruff_organize_imports", "ruff_format" },
      },
    },
  },

  -- ── Python 调试：优先用 PATH 上的 debugpy-adapter（pipx 提供）───────────
  --    LazyVim 默认写死 setup("debugpy-adapter")；若本机没有该脚本会直接报错。
  --    这里做一次防御：debugpy-adapter > python3（需系统装有 debugpy）。
  --    安装：pipx install debugpy   （或 sudo apt install python3-debugpy）
  {
    "mfussenegger/nvim-dap-python",
    optional = true,
    config = function()
      local ok, dap_python = pcall(require, "dap-python")
      if not ok then
        return
      end
      if vim.fn.exepath("debugpy-adapter") ~= "" then
        dap_python.setup("debugpy-adapter")
      elseif vim.fn.exepath("python3") ~= "" then
        dap_python.setup(vim.fn.exepath("python3"))
      else
        vim.notify("未找到 debugpy-adapter / python3，Python 调试不可用", vim.log.levels.WARN)
      end
    end,
  },

  -- ── neotest-python：适配器由 LazyVim 的 lang.python extra 注册──────────
  --    ⚠️ 不要再写一份 { "nvim-neotest/neotest-python", opts = {...} }：
  --    该仓库的适配器插件必须作为 neotest 的 dependency 加载，单独声明会让
  --    lazy 去 require("neotest-python") 并调用 setup()，报错：
  --    loop or previous error loading module 'neotest-python'。
  --    默认就是 runner=pytest + 系统 python3（已安装 python3-pytest），无需额外配置。

  -- ── mason-nvim-dap：关掉自动安装（debugpy 走 pypi，本环境装不上）────────
  --    自动安装逻辑会把 dap.adapters 里的 `python` 映射到 mason 包 debugpy；
  --    本环境 pypi 不可用，会反复失败并附带“退出时打断其他安装”的问题。
  --    debugpy 已由 pipx 提供（PATH 上的 `debugpy-adapter`），codelldb 由 scripts 安装。
  {
    "jay-babu/mason-nvim-dap.nvim",
    optional = true,
    opts = { automatic_installation = false },
  },

  -- ════════════════════════════════════════════════════════════════════════
  --  mason：pypi 类包唯一过滤入口（改由 apt / pipx 提供）
  --    debugpy / basedpyright / python-lsp-server → 走 pypi，本环境会失败
  --    cmakelang / cmakelint / yamllint / cmake-language-server → 同上，已用系统工具替代
  --    安装记录（本机）：
  --      sudo apt install yamllint cmake-format python3-debugpy python3-pytest
  --      pipx install cmakelint ; pipx install debugpy
  -- ════════════════════════════════════════════════════════════════════════
  {
    "mason-org/mason.nvim",
    opts = function(_, opts)
      local pypi_blocked = {
        "cmakelang",
        "cmakelint",
        "cmake_format",
        "cmake-language-server",
        "yamllint",
        "debugpy",
        "basedpyright",
        "python-lsp-server",
      }
      local drop = {}
      for _, name in ipairs(pypi_blocked) do
        drop[name] = true
      end
      opts.ensure_installed = vim.tbl_filter(function(pkg)
        return not drop[pkg]
      end, opts.ensure_installed or {})
      return opts
    end,
  },
}

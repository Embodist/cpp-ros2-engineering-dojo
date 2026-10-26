-- ============================================================================
--  调试：nvim-dap + codelldb（底层即 LLDB）
--  LazyVim dap.core 提供 UI / 虚拟文本；lang.clangd 注册 codelldb adapter 与
--  基础 C/C++ 启动配置。这里补充更实用的启动模板（带参数 / 无参数 / attach）。
--
--  文档：docs/guide/06-debugging-lldb.md
-- ============================================================================

return {
  {
    "mfussenegger/nvim-dap",
    optional = true,
    opts = function()
      local dap = require("dap")
      -- codelldb adapter 由 LazyVim lang.clangd extra 注册（mason 安装 codelldb）
      if not dap.adapters.codelldb then
        return
      end

      for _, lang in ipairs({ "c", "cpp" }) do
        dap.configurations[lang] = {
          {
            name = "启动（带参数）",
            type = "codelldb",
            request = "launch",
            program = function()
              return vim.fn.input("可执行文件: ", vim.fn.getcwd() .. "/build/", "file")
            end,
            args = function()
              local input = vim.fn.input("程序参数: ")
              return vim.split(input, " ", { trimempty = true })
            end,
            cwd = "${workspaceFolder}",
            stopOnEntry = false,
            terminal = "integrated",
          },
          {
            name = "启动（无参数）",
            type = "codelldb",
            request = "launch",
            program = function()
              return vim.fn.input("可执行文件: ", vim.fn.getcwd() .. "/build/", "file")
            end,
            cwd = "${workspaceFolder}",
            stopOnEntry = false,
            terminal = "integrated",
          },
          {
            name = "附加到进程",
            type = "codelldb",
            request = "attach",
            pid = require("dap.utils").pick_process,
            cwd = "${workspaceFolder}",
          },
        }
      end
    end,
  },

  -- 变量值以内联虚拟文本显示，调试时非常直观
  {
    "theHamsta/nvim-dap-virtual-text",
    opts = { commented = true },
  },

  -- 调试面板布局
  {
    "rcarriga/nvim-dap-ui",
    opts = {
      layouts = {
        {
          elements = {
            { id = "scopes", size = 0.4 },
            { id = "breakpoints", size = 0.3 },
            { id = "stacks", size = 0.3 },
            { id = "watches", size = 0.2 },
          },
          size = 40,
          position = "right",
        },
        {
          elements = { { id = "repl", size = 0.6 }, { id = "console", size = 0.4 } },
          size = 0.3,
          position = "bottom",
        },
      },
    },
  },

  -- 确保 codelldb 调试器通过 mason 安装（lang.clangd extra 也会装，这里幂等）
  {
    "jay-babu/mason-nvim-dap.nvim",
    optional = true,
    opts = { ensure_installed = { "codelldb" }, automatic_installation = true },
  },
}

-- ============================================================================
--  C/C++ 主配置
--    · clangd（系统版，mason=false）+ clangd_extensions
--    · clang-format（conform）
--    · CMake 工作流（cmake-tools.nvim）
--  依赖 LazyVim extras：lang.clangd / lang.cmake / dap.core
--  文档：docs/guide/05-clangd-lsp.md · docs/guide/04-cpp-workflow.md
--
--  设计原则：本文件只配置“工具”，不生成你的业务代码。
--            重构建议、错误提示、边界测试由 LSP / clang-tidy / 测试框架给出。
-- ============================================================================

return {
  -- ── Treesitter 解析器 ───────────────────────────────────────────────────
  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = {
        "c",
        "cpp",
        "cmake",
        "bash",
        "lua",
        "luadoc",
        "vim",
        "vimdoc",
        "markdown",
        "markdown_inline",
        "json",
        "yaml",
        "toml",
        "dockerfile",
        "gitignore",
        "diff",
        "query",
        "regex",
      },
    },
  },

  -- ── clangd：使用系统 clangd，避免与 apt/clang 工具链版本割裂 ──────────────
  --    mason = false 表示不从 mason 下载，直接调用 PATH 中的 clangd。
  {
    "neovim/nvim-lspconfig",
    opts = {
      servers = {
        clangd = {
          mason = false,
          cmd = {
            "clangd",
            "--background-index",
            "--background-index-priority=normal",
            "--clang-tidy",
            "--completion-style=detailed",
            "--header-insertion=iwyu",
            "--header-insertion-decorators",
            "--function-arg-placeholders",
            "--pch-storage=memory",
            "--all-scopes-completion",
            "--limit-results=0",
            "--enable-config",
            "--fallback-style=Google",
            "--query-driver=/usr/bin/g++*,/usr/bin/gcc*,/usr/bin/clang++*,/usr/bin/clang*",
          },
          init_options = {
            usePlaceholders = true,
            completeUnimported = true,
            clangdFileStatus = true,
          },
          keys = {
            { "<leader>ch", "<cmd>LspClangdSwitchSourceHeader<cr>", desc = "C/C++ 头/源文件切换" },
          },
        },
      },
    },
  },

  -- ── clangd 扩展：AST / 类型层级 / 内存占用 / 符号信息 ────────────────────
  {
    "p00f/clangd_extensions.nvim",
    ft = { "c", "cpp", "objc", "objcpp", "cuda" },
    keys = {
      { "<leader>cM", "<cmd>ClangdMemoryUsage<cr>", desc = "clangd 内存占用" },
      { "<leader>cH", "<cmd>ClangdTypeHierarchy<cr>", desc = "clangd 类型层级" },
      { "<leader>cN", "<cmd>ClangdSymbolInfo<cr>", desc = "clangd 符号信息" },
      { "<leader>cS", "<cmd>ClangdAST<cr>", desc = "clangd AST（当前文件）" },
    },
    opts = {
      inlay_hints = {
        inline = true, -- C++ 里 inline 显示参数名/类型推导，很实用（用 <leader>uh 开关）
      },
      ast = {
        role_icons = {},
        kind_icons = {},
      },
      extensions = {
        autoSetHints = true,
        hover_with_actions = true,
        show_message = "on",
      },
    },
  },

  -- ── clang-format：优先读取项目 .clang-format ─────────────────────────────
  --    保存时自动格式化由 LazyVim 的 format-on-save 控制（<leader>uf 开关）
  {
    "stevearc/conform.nvim",
    opts = {
      formatters_by_ft = {
        c = { "clang-format" },
        cpp = { "clang-format" },
        cuda = { "clang-format" },
      },
      formatters = {
        ["clang-format"] = {
          prepend_args = { "--style=file", "--fallback-style=Google" },
        },
      },
    },
  },

  -- ── CMake：生成 / 构建 / 运行 / 测试 ─────────────────────────────────────
  --    键位经核对，避开 LazyVim 已占用的 c{a,c,cC,cR,cr,cA,co,cm,cf,cd,cl,ch,cs}
  {
    "Civitasv/cmake-tools.nvim",
    keys = {
      { "<leader>cg", "<cmd>CMakeGenerate<cr>", desc = "CMake 生成 (configure)" },
      { "<leader>cb", "<cmd>CMakeBuild<cr>", desc = "CMake 构建" },
      { "<leader>cB", "<cmd>CMakeQuickBuild<cr>", desc = "CMake 快速构建" },
      { "<leader>cx", "<cmd>CMakeRun<cr>", desc = "CMake 运行" },
      { "<leader>cX", "<cmd>CMakeQuickRun<cr>", desc = "CMake 快速运行" },
      { "<leader>cT", "<cmd>CMakeRunTest<cr>", desc = "CMake 测试 (ctest)" },
      { "<leader>cv", "<cmd>CMakeSelectBuildType<cr>", desc = "CMake 构建类型" },
      { "<leader>cL", "<cmd>CMakeSelectLaunchTarget<cr>", desc = "CMake 运行目标" },
      { "<leader>cP", "<cmd>CMakeSelectBuildPreset<cr>", desc = "CMake 构建预设" },
      { "<leader>cJ", "<cmd>CMakeLaunchArgs<cr>", desc = "CMake 运行参数" },
      { "<leader>cK", "<cmd>CMakeClean<cr>", desc = "CMake 清理" },
    },
    opts = {
      cmake_build_directory = "build",
      -- 关键：导出 compile_commands.json 供 clangd 使用
      cmake_generate_options = { "-DCMAKE_EXPORT_COMPILE_COMMANDS=1" },
      cmake_executor = { name = "quickfix" }, -- 构建日志进 quickfix
      cmake_runner = { name = "terminal" }, -- 运行用内置终端
      cmake_virtual_text_support = true,
    },
  },

  -- ── mason：pypi 类包统一由系统提供（apt / pipx）────────────────────────
  --    统一的过滤表在 lua/plugins/langs.lua（单一入口，避免多处重复）。
  --    这里只保留 CMake/YAML 侧的说明：cmakelang(cmake-format) / cmakelint / yamllint
  --    改用系统工具（apt yamllint cmake-format、pipx cmakelint）。
}

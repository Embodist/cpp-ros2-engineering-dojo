-- ============================================================================
--  数据文件（JSON / YAML / XML / TOML）工具链
--    · SchemaStore.nvim    → 为 jsonls / yamlls 提供官方 Schema（补全 + 校验）
--    · nvim-ufo            → 更聪明的折叠（treesitter / LSP provider）
--    · nvim-jqx            → 用 jq 交互式浏览 / 查询 JSON、YAML
--    · conform             → XML / TOML 格式化（json/yaml 由 prettier 负责）
--    · nvim-lint           → yamllint / xmllint 静态检查
--
--  快捷操作（见 config/keymaps.lua，<leader>j 分组）：
--    <leader>jf  格式化        <leader>js  格式化并排序键
--    <leader>jm  压缩          <leader>jv  校验
--    <leader>jj  jqx 浏览      <leader>jq  jqx 查询
--  文档：docs/guide/16-data-formats.md
-- ============================================================================

return {
  -- ── Schema 补全（package.json / docker-compose.yml / .clang-format ...）──
  {
    "b0o/SchemaStore.nvim",
    lazy = true,
    version = false,
  },

  {
    "neovim/nvim-lspconfig",
    dependencies = { "b0o/SchemaStore.nvim" },
    opts = function(_, opts)
      opts.servers = opts.servers or {}
      local ok, schemastore = pcall(require, "schemastore")
      if not ok then
        return opts
      end

      opts.servers.jsonls = vim.tbl_deep_extend("force", opts.servers.jsonls or {}, {
        settings = {
          json = {
            schemas = schemastore.json.schemas(),
            validate = { enable = true },
          },
        },
      })

      opts.servers.yamlls = vim.tbl_deep_extend("force", opts.servers.yamlls or {}, {
        settings = {
          yaml = {
            -- 用 SchemaStore 的 schema（关闭 yamlls 自带的 schemaStore 以免重复）
            schemaStore = { enable = false, url = "" },
            schemas = schemastore.yaml.schemas(),
            validate = true,
          },
        },
      })

      return opts
    end,
  },

  -- ── Treesitter：补 XML 解析器 ────────────────────────────────────────────
  {
    "nvim-treesitter/nvim-treesitter",
    opts = {
      ensure_installed = { "xml", "json5", "dot" },
    },
  },

  -- ── 折叠：见 config/options.lua（treesitter 内置 foldexpr；nvim-ufo 已禁用）──────
  {
    "kevinhwang91/nvim-ufo",
    -- ⚠️ 禁用（实测冲突）：nvim-ufo 需要 kevinhwang91/promise-async
    --    （模块 `async` = 可调用表），而 LazyVim 的 editor.refactoring extra
    --    需要 lewis6991/async.nvim（同样提供 lua/async.lua，但是 async.run/async.await
    --    的普通表）。两者共用模块名 `async`，谁先入 runtimepath 谁赢；
    --    一旦 async.nvim 先加载，nvim-ufo 折叠就报
    --    "attempt to call upvalue 'async' (a table value)"，每开一个文件都报。
    --    改用 Neovim 0.12 内置 vim.treesitter.foldexpr()（见 config/options.lua），
    --    za/zR/zM/zr/zm 均为内置键位，功能不缺。
    enabled = false,
  },

  -- ── jq 交互式浏览 / 查询 JSON 与 YAML ───────────────────────────────────
  {
    "gennaro-tedesco/nvim-jqx",
    ft = { "json", "yaml" },
    keys = {
      { "<leader>jj", "<cmd>JqxList<cr>", desc = "jqx: 浏览 JSON/YAML", ft = { "json", "yaml" } },
      { "<leader>jq", "<cmd>JqxQuery<cr>", desc = "jqx: jq 查询", ft = { "json", "yaml" } },
    },
  },

  -- ── conform：XML / TOML 格式化（全局 formatters_by_ft 合并）──────────────
  {
    "stevearc/conform.nvim",
    opts = {
      formatters_by_ft = {
        xml = { "xmllint" },
        toml = { "taplo" },
      },
      formatters = {
        xmllint = {
          command = "xmllint",
          args = { "--format", "-" },
          stdin = true,
        },
      },
    },
  },

  -- ── Lint：yamllint / xmllint ────────────────────────────────────────────
  {
    "mfussenegger/nvim-lint",
    opts = function(_, opts)
      opts.linters_by_ft = opts.linters_by_ft or {}
      opts.linters_by_ft.yaml = { "yamllint" }
      opts.linters_by_ft.xml = { "xmllint" }
      return opts
    end,
  },
  {
    "mason-org/mason.nvim",
    -- yamllint 由系统 apt 提供（见 cpp.lua 的 mason override，避免 pypi 反复失败）
    opts = { ensure_installed = { "taplo" } },
  },
}

-- ============================================================================
--  AI 辅助 —— 默认【全部关闭】
--
--  设计原则（按用户要求）：
--    · 默认不使用 AI 补全，也不会主动弹出任何 AI 提示；
--    · 代码报错 / 代码动作 / 类型提示 全部交给 LSP
--      （clangd 诊断、inlay hints、nvim-lightbulb 的代码动作提示）；
--    · 需要 AI 时再手动开启：  NVIM_ENABLE_AI=1 nvim
--
--  开启后可用的三种角色（互不重复）：
--    CodeCompanion  编辑器内对话 / 内联改写（默认 DeepSeek，可切 Ollama 本地）
--    Copilot        行内幽灵补全（需订阅，:Copilot auth）
--    sidekick       驱动 CLI Agent（Claude Code 等），NES 默认仍关闭
--
--  文档：docs/guide/09-ai-assist.md
-- ============================================================================

-- 总开关：只有显式设置 NVIM_ENABLE_AI=1 时才加载任何 AI 插件
local AI = vim.env.NVIM_ENABLE_AI == "1"

return {
  -- ── CodeCompanion：对话 / 内联改写（手动触发，不自动补全）──────────────
  {
    "olimorris/codecompanion.nvim",
    enabled = AI,
    dependencies = {
      "nvim-lua/plenary.nvim",
      "nvim-treesitter/nvim-treesitter",
    },
    cmd = {
      "CodeCompanion",
      "CodeCompanionChat",
      "CodeCompanionActions",
      "CodeCompanionCmd",
    },
    keys = {
      { "<leader>ac", "<cmd>CodeCompanionChat Toggle<cr>", desc = "AI: 对话开关", mode = { "n", "v" } },
      { "<leader>aC", "<cmd>CodeCompanionChat Add<cr>", desc = "AI: 把当前缓冲区加入对话", mode = { "n", "v" } },
      { "<leader>aA", "<cmd>CodeCompanionActions<cr>", desc = "AI: 动作面板", mode = { "n", "v" } },
      { "<leader>ai", "<cmd>CodeCompanion<cr>", desc = "AI: 内联改写", mode = { "n", "v" } },
      { "<leader>ae", "<cmd>CodeCompanionChat Explain<cr>", desc = "AI: 解释选中代码", mode = { "v" } },
      { "<leader>ar", "<cmd>CodeCompanionChat Review<cr>", desc = "AI: 代码审查", mode = { "v" } },
      { "<leader>ag", "<cmd>CodeCompanionChat Tests<cr>", desc = "AI: 生成边界测试", mode = { "v" } },
    },
    opts = {
      interactions = {
        chat = { adapter = "deepseek" },
        inline = { adapter = "deepseek" },
      },
      -- 想用本地模型（完全离线）：把上面的 adapter 改成 "ollama"
      -- 旧版本 CodeCompanion 使用 strategies 字段，若版本较旧请改回。
    },
  },

  -- ── Copilot：行内补全（额外需要 NVIM_NO_COPILOT != 1 与 :Copilot auth）──
  --    ⚠️ 默认不加载；即使 AI=1，也需自己执行 :Copilot auth 才会真正启用
  {
    "zbirenbaum/copilot.lua",
    enabled = AI and vim.env.NVIM_NO_COPILOT ~= "1",
    build = function()
      vim.notify("如需 Copilot：在 Neovim 中执行 :Copilot auth 完成授权", vim.log.levels.INFO)
    end,
    opts = {
      suggestion = { enabled = true, auto_trigger = true },
      panel = { enabled = false },
      filetypes = { cpp = true, c = true, cmake = true, markdown = true },
    },
  },

  -- ── sidekick：CLI Agent（Claude Code 等）；NES 保持关闭 ─────────────────
  {
    "folke/sidekick.nvim",
    enabled = AI,
    opts = {
      nes = { enabled = false }, -- 关闭“下一步编辑建议”，避免默认打扰
    },
  },
}

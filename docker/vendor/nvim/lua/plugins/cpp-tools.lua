-- ============================================================================
--  C++ 专用代码生成 / 文档注释
--    · nvim-treesitter-cpp-tools → 由声明生成定义、Rule of 3/5、具体类实现
--    · neogen                    → 生成 Doxygen 文档注释
--
--  键位统一放在 <leader>n（geNerate）前缀，避开已占用的 c/k/j/a 等前缀。
--  文档：docs/guide/17-efficient-plugins.md
--
--  ⚠️ 这些工具用于**减少机械劳动**（写重复的 getter/定义），
--     但生成出来的签名/语义仍需你确认是否正确。
-- ============================================================================

return {
  {
    "Badhi/nvim-treesitter-cpp-tools",
    dependencies = { "nvim-treesitter/nvim-treesitter" },
    ft = { "cpp" },
    keys = {
      -- 光标放在类的成员函数**声明**上执行
      { "<leader>ni", "<cmd>TSCppDefineClassFunc<cr>", desc = "C++: 由声明生成函数定义" },
      { "<leader>nc", "<cmd>TSCppMakeConcreteClass<cr>", desc = "C++: 生成具体类实现" },
      { "<leader>n3", "<cmd>TSCppRuleOf3<cr>", desc = "C++: 生成 Rule of 3" },
      { "<leader>n5", "<cmd>TSCppRuleOf5<cr>", desc = "C++: 生成 Rule of 5" },
    },
    opts = {
      -- 生成前先弹出预览（强烈建议保留：先看清再应用）
      preview = { quit = "q" },
    },
  },

  {
    "danymat/neogen",
    dependencies = { "nvim-treesitter/nvim-treesitter" },
    keys = {
      { "<leader>nd", function() require("neogen").generate() end, desc = "生成 Doxygen 文档注释" },
    },
    opts = {
      snippet_engine = "luasnip",
      enabled = true,
    },
  },
}

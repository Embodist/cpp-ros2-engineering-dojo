-- ============================================================================
--  单元测试：Neotest + GoogleTest 适配器
--  LazyVim test.core 提供 Neotest 与 <leader>t* 键位；这里挂接 C++ 适配器。
--
--  常用键位（LazyVim 提供）：
--    <leader>tt  运行当前测试文件
--    <leader>tr  运行光标所在测试
--    <leader>ts  测试摘要
--    <leader>to  查看输出
--    <leader>td  调试最近测试（DAP）
--
--  文档：docs/guide/07-testing.md
-- ============================================================================

return {
  {
    "nvim-neotest/neotest",
    dependencies = {
      -- GoogleTest 适配器（保留 alfaix 维护版）
      "alfaix/neotest-gtest",
    },
    opts = function(_, opts)
      opts.adapters = opts.adapters or {}

      -- 兼容不同版本适配器导出方式：
      --   require("neotest-gtest") / .adapter / .adapter(cfg)
      local ok, adapter = pcall(function()
        local gt = require("neotest-gtest")
        if type(gt) == "function" then
          return gt({})
        end
        if type(gt) == "table" and gt.adapter then
          return type(gt.adapter) == "function" and gt.adapter({}) or gt.adapter
        end
        return gt
      end)

      if ok and adapter then
        table.insert(opts.adapters, adapter)
      else
        vim.notify(
          "neotest-gtest 适配器加载失败，请确认依赖已安装（:Lazy sync）",
          vim.log.levels.WARN
        )
      end

      return opts
    end,
  },
}

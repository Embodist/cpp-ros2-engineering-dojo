-- ============================================================================
--  竞赛 / 刷题
--    · CompetiTest：单文件编译 + 样例测试 + diff + 压力测试
--    · LeetCode.nvim：题库浏览 / 运行 / 提交
--  文档：docs/guide/08-leetcode-competitive.md
--
--  键位说明：为避免与 LazyVim 的 LSP 键位冲突（<leader>cr 重命名、<leader>cd 诊断），
--            竞赛相关统一放在 <leader>k 前缀（k = kontest，便于记忆）。
-- ============================================================================

return {
  -- ── CompetiTest ─────────────────────────────────────────────────────────
  {
    "xeluxee/competitest.nvim",
    dependencies = { "MunifTanjim/nui.nvim" },
    keys = {
      { "<leader>kr", "<cmd>CompetiTest run<cr>", desc = "竞赛: 编译并跑全部样例" },
      { "<leader>kl", "<cmd>CompetiTest run_no_compile<cr>", desc = "竞赛: 直接跑（不重编译）" },
      { "<leader>ko", "<cmd>CompetiTest show_ui<cr>", desc = "竞赛: 打开测试面板" },
      { "<leader>ke", "<cmd>CompetiTest edit_testcase<cr>", desc = "竞赛: 编辑测试用例" },
      { "<leader>ka", "<cmd>CompetiTest add_testcase<cr>", desc = "竞赛: 新增测试用例" },
      { "<leader>kt", "<cmd>CompetiTest receive testcases<cr>", desc = "竞赛: 接收测试用例" },
      { "<leader>kp", "<cmd>CompetiTest receive problem<cr>", desc = "竞赛: 接收题目" },
      { "<leader>kc", "<cmd>CompetiTest receive contest<cr>", desc = "竞赛: 接收整场比赛" },
    },
    opts = {
      -- 编译：C++23；-DLOCAL 方便你写 #ifdef LOCAL 的调试宏
      compile_command = {
        cpp = {
          exec = "g++",
          args = { "-std=c++23", "-O2", "-Wall", "-Wextra", "-DLOCAL", "$(FNAME)", "-o", "$(FNOEXT)" },
        },
        c = {
          exec = "gcc",
          args = { "-std=c17", "-O2", "-Wall", "-Wextra", "-DLOCAL", "$(FNAME)", "-o", "$(FNOEXT)" },
        },
      },
      run_command = {
        cpp = { exec = "./$(FNOEXT)" },
        c = { exec = "./$(FNOEXT)" },
      },
      running_directory = ".",
      testcases_directory = "testcases",
      testcases_use_single_file = false, -- 每个样例独立 .in/.out 文件
      output_compare_method = "squish", -- 忽略行尾/多余空白
      view_output_diff = true,
      maximum_time = 5000, -- 超时 5s
      multiple_testing = -1, -- 并行跑所有样例
      save_current_file = true,
      runner_ui = { interface = "popup" },
      -- 接收题目（配合浏览器插件 Competitive Companion）保存位置
      received_problems_path = "$(HOME)/competitive/$(PROBLEM)",
      received_contests_directory = "$(HOME)/competitive/contests",
    },
  },

  -- ── LeetCode.nvim ───────────────────────────────────────────────────────
  --    首次使用需登录：在 LeetCode 网页复制 cookie，然后 :Leet cookie update
  {
    "kawre/leetcode.nvim",
    dependencies = {
      "nvim-telescope/telescope.nvim",
      "nvim-lua/plenary.nvim",
      "MunifTanjim/nui.nvim",
    },
    build = ":TSUpdate html markdown markdown_inline",
    cmd = "Leet",
    keys = {
      { "<leader>ll", "<cmd>Leet<cr>", desc = "LeetCode: 主菜单" },
      { "<leader>lp", "<cmd>Leet list<cr>", desc = "LeetCode: 选题" },
      { "<leader>lR", "<cmd>Leet random<cr>", desc = "LeetCode: 随机题" },
      { "<leader>ld", "<cmd>Leet desc toggle<cr>", desc = "LeetCode: 题目描述" },
      { "<leader>lr", "<cmd>Leet run<cr>", desc = "LeetCode: 运行" },
      { "<leader>lt", "<cmd>Leet test<cr>", desc = "LeetCode: 测试" },
      { "<leader>ls", "<cmd>Leet submit<cr>", desc = "LeetCode: 提交" },
      { "<leader>lD", "<cmd>Leet daily<cr>", desc = "LeetCode: 每日一题" },
      { "<leader>li", "<cmd>Leet info<cr>", desc = "LeetCode: 题目信息" },
      { "<leader>lh", "<cmd>Leet hints<cr>", desc = "LeetCode: 提示" },
      { "<leader>ly", "<cmd>Leet yank<cr>", desc = "LeetCode: 复制代码" },
      { "<leader>lo", "<cmd>Leet open<cr>", desc = "LeetCode: 浏览器打开" },
      { "<leader>lx", "<cmd>Leet exit<cr>", desc = "LeetCode: 退出" },
    },
    opts = {
      lang = "cpp",
      is_cn = false, -- 用国际站；国内站改为 true
      storage = { home = vim.fn.expand("~/leetcode") },
      picker = { provider = "telescope" },
      -- 关掉自动打开 console/描述，专注写代码
      console = { open_on_running = false },
    },
  },
}

-- ============================================================================
--  自动命令
--  LazyVim 会先加载自己的 autocmds，再加载本文件。
-- ============================================================================

local augroup = vim.api.nvim_create_augroup("UserConfig", { clear = true })

-- ── 回到上次编辑位置 ─────────────────────────────────────────────────────
vim.api.nvim_create_autocmd("BufReadPost", {
  group = augroup,
  callback = function()
    local mark = vim.api.nvim_buf_get_mark(0, '"')
    local lcount = vim.api.nvim_buf_line_count(0)
    if mark[1] > 0 and mark[1] <= lcount then
      pcall(vim.api.nvim_win_set_cursor, 0, mark)
    end
  end,
})

-- ── 大文件降级（>1MB 关闭 treesitter 高亮 / 折叠 / 撤销等重功能）──────────
vim.api.nvim_create_autocmd("BufReadPre", {
  group = augroup,
  callback = function(args)
    local ok, stats = pcall(vim.uv.fs_stat, vim.api.nvim_buf_get_name(args.buf))
    if ok and stats and stats.size > 1024 * 1024 then
      vim.b[args.buf].bigfile = true
      vim.opt_local.foldmethod = "manual"
      vim.opt_local.spell = false
      vim.opt_local.swapfile = false
      vim.opt_local.undofile = false
    end
  end,
})

-- ── Markdown / Git commit 打开拼写与自动换行 ─────────────────────────────
vim.api.nvim_create_autocmd("FileType", {
  group = augroup,
  pattern = { "markdown", "gitcommit", "text" },
  callback = function()
    vim.opt_local.spell = true
    vim.opt_local.wrap = true
    vim.opt_local.linebreak = true
  end,
})

-- ── C/C++：使用 // 注释，避免 formatoptions 自动插入连续注释 ──────────────
vim.api.nvim_create_autocmd("FileType", {
  group = augroup,
  pattern = { "c", "cpp", "cuda", "objc", "objcpp" },
  callback = function(args)
    vim.opt_local.commentstring = "// %s"
    vim.opt_local.formatoptions:remove({ "c", "r", "o" })
    -- 单文件编译运行：<C-r> 编译并跑（tmp.out）· <leader>cc 只编译 · <leader>cC 只跑
    -- 只在 C/C++ 缓冲区占 <C-r>，普通缓冲区保留 Vim 内置 redo。
    require("util.cpprun").attach(args.buf)
  end,
})

-- ── 竞赛文件（常见命名）：提示可直接 <leader>kr 运行 ─────────────────────
-- 说明：这里刻意不自动创建/修改文件系统，保持“代码由你自己写”的原则。
vim.api.nvim_create_autocmd("FileType", {
  group = augroup,
  pattern = { "cpp", "c" },
  callback = function(args)
    local name = vim.api.nvim_buf_get_name(args.buf)
    if name:match("[/\\]testcases[/\\]") or name:match("[/\\]lc[/\\]") then
      vim.b[args.buf].competitest_hint = true
    end
  end,
})

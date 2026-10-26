-- ============================================================================
--  编辑器选项
--  加载顺序：LazyVim 默认选项 -> 本文件（本文件可覆盖 LazyVim 默认值）
--  参考：docs/guide/03-keymaps.md
-- ============================================================================

local opt = vim.opt

-- ── 行号 / 光标 ────────────────────────────────────────────────────────────
opt.number = true
opt.relativenumber = true
opt.cursorline = true
opt.scrolloff = 8
opt.sidescrolloff = 8

-- ── 缩进：C++ 工程常用 4 空格（clang-format 会按项目 .clang-format 覆盖）──
opt.expandtab = true
opt.shiftwidth = 4
opt.tabstop = 4
opt.softtabstop = 4
opt.smartindent = true

-- ── 搜索 ──────────────────────────────────────────────────────────────────
opt.ignorecase = true
opt.smartcase = true
opt.hlsearch = true
opt.incsearch = true

-- ── 界面 ──────────────────────────────────────────────────────────────────
opt.wrap = false
opt.termguicolors = true
opt.signcolumn = "yes"
opt.colorcolumn = "100"
opt.splitright = true
opt.splitbelow = true
opt.laststatus = 3
opt.showmode = false
opt.list = true
opt.listchars = { tab = "» ", trail = "·", nbsp = "␣" }

-- ── 行为 ──────────────────────────────────────────────────────────────────
opt.mouse = "a"
opt.clipboard = "unnamedplus"
opt.undofile = true            -- 持久化撤销
opt.undolevels = 10000
opt.updatetime = 200           -- 更快触发 gitsigns / diagnostics
opt.timeoutlen = 300
opt.confirm = true             -- 退出前确认保存
opt.autoread = true            -- 文件被外部修改时自动重载
opt.inccommand = "split"       -- 替换实时预览
opt.virtualedit = "block"
opt.iskeyword:append("-")      -- 让 foo-bar 视为一个词

-- ── 折叠：Neovim 0.12 内置 treesitter foldexpr（不用 nvim-ufo，原因见 plugins/data.lua）──
opt.foldmethod = "expr"
opt.foldexpr = "v:lua.vim.treesitter.foldexpr()"
opt.foldtext = ""
opt.foldcolumn = "1"
opt.foldlevel = 99
opt.foldlevelstart = 99
opt.foldenable = true

-- ── 大文件 / 性能提示 ─────────────────────────────────────────────────────
-- 见 lua/config/autocmds.lua 的 BufReadPre 处理

-- ============================================================================
--  主题（配色）管理 —— **想改主题只改这个文件**
--
--  用法：
--    <leader>uM   切到下一个主题（实时生效，并记住选择）
--    <leader>um   切到上一个主题
--    <leader>uC   打开主题选择器（滚动即实时预览，LazyVim 自带）
--    :Theme <name>   （可选）直接指定主题
--
--  重启后仍是上次选的主题（记录在 stdpath("state")/theme）。
--  文档：docs/guide/22-theme.md
-- ============================================================================

local M = {}

-- ── ① 主题候选列表 ────────────────────────────────────────────────────────
--  <leader>uM / <leader>um 按此顺序循环；**列表第一项就是默认主题**。
--  增删主题：改这个列表即可（名字 = `:colorscheme` 的名字）。
--  插件安装：见 nvim/lua/plugins/theme.lua（本文件只管“用哪个”）。
M.themes = {
  "tokyonight", -- 默认：LazyVim 自带（style=moon，见 LazyVim colorscheme.lua）
  "catppuccin", -- LazyVim 自带（flavour=mocha）
  "kanagawa", -- 本仓库新增（见 plugins/theme.lua）
  "habamax", -- Neovim 内置，保底（插件缺失时也能切）
}

-- 常用变体名也可以直接写进列表（lazy 会按需加载对应插件）：
--   "tokyonight-night" / "tokyonight-storm" / "catppuccin-latte" / "kanagawa-dragon"

-- ── ② 状态文件（记住上次选择）────────────────────────────────────────────
function M.state_file()
  return vim.fn.stdpath("state") .. "/theme"
end

--- 当前（或上次）主题名
function M.current()
  local ok, lines = pcall(vim.fn.readfile, M.state_file())
  local name = ok and lines and lines[1] or nil
  if type(name) == "string" and name ~= "" then
    return name
  end
  return M.themes[1]
end

-- ── ③ 扩展点：主题应用后想加“私货”就写在这里 ──────────────────────────────
---@param name string 刚应用的主题名
function M.on_apply(name)
  -- 例 1：透明背景（配合终端透明）
  -- if vim.g.transparent_bg then
  --   for _, grp in ipairs({ "Normal", "NormalNC", "SignColumn", "EndOfBuffer", "NormalFloat" }) do
  --     vim.api.nvim_set_hl(0, grp, { bg = "none" })
  --   end
  -- end
  --
  -- 例 2：只改某个主题的个别高亮
  -- if name == "kanagawa" then
  --   vim.api.nvim_set_hl(0, "Comment", { italic = true })
  -- end
  local _ = name
end

-- ── ④ 应用 / 切换（内部）──────────────────────────────────────────────────
--- 只应用，不持久化；返回是否成功
---@param name string
function M.apply(name)
  if type(name) ~= "string" or name == "" then
    return false
  end
  local ok = pcall(vim.cmd.colorscheme, name)
  if not ok then
    vim.notify("主题不可用：" .. name .. "（检查 plugins/theme.lua 是否已安装）", vim.log.levels.WARN)
    return false
  end
  pcall(M.on_apply, name)
  vim.api.nvim_exec_autocmds("User", { pattern = "ThemeChanged", data = name })
  return true
end

--- 应用并记住（重启后仍然生效）
---@param name string
function M.set(name)
  if M.apply(name) then
    pcall(vim.fn.mkdir, vim.fn.stdpath("state"), "p")
    pcall(vim.fn.writefile, { name }, M.state_file())
    vim.notify("主题：" .. name .. "（已保存，重启保持）", vim.log.levels.INFO)
  end
end

--- 按列表循环切换
---@param step integer? 默认 1（下一个）
function M.next(step)
  step = step or 1
  local list = M.themes
  if #list == 0 then
    return
  end
  local cur = M.current()
  local idx = 1
  for i, n in ipairs(list) do
    if n == cur then
      idx = i
    end
  end
  M.set(list[((idx - 1 + step) % #list) + 1])
end

-- ── ⑤ 启动时调用（LazyVim 的 colorscheme 钩子里已接好）───────────────────
function M.load()
  M.apply(M.current())
end

-- 顺手提供 :Theme 命令（:Theme 列出全部 / :Theme kanagawa 切换）
vim.api.nvim_create_user_command("Theme", function(args)
  if args.args == "" then
    local cur = M.current()
    local lines = { "可用主题（当前：" .. cur .. "）：" }
    for _, n in ipairs(M.themes) do
      lines[#lines + 1] = (n == cur and "  * " or "    ") .. n
    end
    lines[#lines + 1] = "用法：:Theme <name> 切换并保存"
    vim.notify(table.concat(lines, "\n"), vim.log.levels.INFO)
  else
    M.set(args.args)
  end
end, { nargs = "?", complete = function() return M.themes end, desc = "切换主题（无参数则列出）" })

return M

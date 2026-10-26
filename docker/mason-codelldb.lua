-- 用 -u NONE 干净启动，手动加载 mason.nvim 的 registry，
-- 完全绕开 LazyVim/mason-nvim-dap 的自动安装竞态。
local data = vim.fn.stdpath("data")
for _, name in ipairs({ "plenary.nvim", "nui.nvim", "mason.nvim" }) do
  local dir = data .. "/lazy/" .. name
  if vim.fn.isdirectory(dir) == 1 then
    vim.opt.rtp:append(dir)
  end
end

local ok_mason, mason = pcall(require, "mason")
if ok_mason then
  pcall(mason.setup, {})
end

local ok_reg, registry = pcall(require, "mason-registry")
if not ok_reg then
  print("[mason] registry unavailable")
  vim.cmd("qa!")
  return
end

local function installed()
  local ok, pkg = pcall(registry.get_package, "codelldb")
  return ok and pkg:is_installed()
end

local function start()
  local ok, pkg = pcall(registry.get_package, "codelldb")
  if not ok then
    print("[mason] codelldb not in registry")
    vim.cmd("qa!")
    return
  end
  if pkg:is_installed() then
    print("[mason] codelldb already installed")
    vim.cmd("qa!")
    return
  end

  print("[mason] installing codelldb")
  pkg:install()
  local deadline = os.time() + 2100
  while os.time() < deadline do
    if installed() then
      break
    end
    vim.wait(2000)
  end
  print(installed() and "[mason] codelldb OK" or "[mason] codelldb missing")
  vim.cmd("qa!")
end

local ok_refresh = pcall(registry.refresh, function()
  start()
end)
if not ok_refresh then
  start()
end

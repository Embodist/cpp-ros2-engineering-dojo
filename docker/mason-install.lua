-- 在已加载 Mason 注册表的 headless nvim 中补装 LSP/工具。
-- 不依赖 LazyVim 的自动安装队列，逐包等待 is_installed()。
local packages = {
  "lua-language-server",
  "pyright",
  "vtsls",
  "bash-language-server",
  "json-lsp",
  "yaml-language-server",
  "marksman",
  "cmake-language-server",
  "dockerfile-language-server",
  "stylua",
  "codelldb",
  "hadolint",
  "debugpy",
  "ruff",
  "prettier",
  "markdownlint-cli2",
  "markdown-toc",
  "cmakelang",
  "cmakelint",
  "taplo",
}

local registry = require("mason-registry")

local function installed(name)
  local ok, pkg = pcall(registry.get_package, name)
  return ok and pkg:is_installed()
end

local function all_installed()
  for _, name in ipairs(packages) do
    if not installed(name) then
      return false
    end
  end
  return true
end

local function start()
  for _, name in ipairs(packages) do
    local ok, pkg = pcall(registry.get_package, name)
    if ok and not pkg:is_installed() then
      print("[mason] installing " .. name)
      pkg:install()
    end
  end

  local deadline = os.time() + 900
  while os.time() < deadline do
    if all_installed() then
      print("[mason] all packages installed")
      break
    end
    vim.wait(2000)
  end

  for _, name in ipairs(packages) do
    if not installed(name) then
      print("[mason] missing " .. name)
    end
  end
  vim.cmd("qa!")
end

local ok = pcall(registry.refresh, function()
  start()
end)
if not ok then
  start()
end

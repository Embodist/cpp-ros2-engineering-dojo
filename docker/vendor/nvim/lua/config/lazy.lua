-- ============================================================================
--  lazy.nvim 引导 + LazyVim 装配
--  文档：docs/guide/01-installation.md
--
--  离线部署：设置环境变量 NVIM_OFFLINE=1 时，不会尝试联网克隆 lazy.nvim，
--  而是直接使用 scripts/restore-offline.sh 从 vendor/ 恢复出来的插件缓存。
-- ============================================================================

local lazypath = vim.fn.stdpath("data") .. "/lazy/lazy.nvim"
local offline = vim.env.NVIM_OFFLINE == "1" or vim.g.nvim_offline == true

if not (vim.uv or vim.loop).fs_stat(lazypath) then
  if offline then
    vim.notify(
      "NVIM_OFFLINE=1 但未找到 lazy.nvim：\n" .. lazypath .. "\n请先运行 scripts/restore-offline.sh",
      vim.log.levels.ERROR
    )
    os.exit(1)
  end

  local lazyrepo = "https://github.com/folke/lazy.nvim.git"
  local out = vim.fn.system({ "git", "clone", "--filter=blob:none", "--branch=stable", lazyrepo, lazypath })
  if vim.v.shell_error ~= 0 then
    vim.api.nvim_echo({
      { "Failed to clone lazy.nvim:\n", "ErrorMsg" },
      { out, "WarningMsg" },
      { "\nPress any key to exit..." },
    }, true, {})
    vim.fn.getchar()
    os.exit(1)
  end
end
vim.opt.rtp:prepend(lazypath)

require("lazy").setup({
  spec = {
    -- LazyVim 本体与默认插件集
    { "LazyVim/LazyVim", import = "lazyvim.plugins" },
    -- 本仓库的自定义插件（按 lua/plugins/*.lua 自动导入）
    { import = "plugins" },
  },
  defaults = {
    -- 自定义插件默认随启动加载；lazy 加载由各自 spec 的 event/ft/keys 决定
    lazy = false,
    -- LazyVim 建议保持 version = false（很多插件的 release 版本过旧）
    version = false,
  },
  install = {
    colorscheme = { "tokyonight", "habamax" },
    missing = true,
  },
  checker = {
    enabled = not offline, -- 离线模式不做更新检查
    notify = false,
  },
  change_detection = { notify = false },
  performance = {
    rtp = {
      disabled_plugins = {
        "gzip",
        "tarPlugin",
        "tohtml",
        "tutor",
        "zipPlugin",
      },
    },
  },
  ui = { border = "rounded" },
})

-- ============================================================================
--  ROS 2 / 机器人 文件类型支持
--    .urdf .xacro .sdf .srdf .mjcf  -> xml
--    *.launch / .launch.xml         -> xml
--    *.launch.py                    -> python
--    .msg .srv .action              -> rosidl（自定义 ft，注释为 #）
--    .repos                         -> yaml
--    package.xml                    -> xml
--
--  keymaps 见 lua/config/keymaps.lua 的 <leader>R 分组
--  文档：docs/guide/23-robotics-embodied-ai.md
-- ============================================================================

vim.filetype.add({
  extension = {
    urdf = "xml",
    xacro = "xml",
    sdf = "xml",
    srdf = "xml",
    mjcf = "xml",
    launch = "xml", -- ROS 1 XML launch
    repos = "yaml",
    msg = "rosidl",
    srv = "rosidl",
    action = "rosidl",
  },
  filename = {
    ["package.xml"] = "xml",
  },
  pattern = {
    [".*%.launch%.xml"] = "xml",
    [".*%.launch%.py"] = "python",
  },
})

vim.api.nvim_create_autocmd("FileType", {
  group = vim.api.nvim_create_augroup("RosFiletypes", { clear = true }),
  pattern = { "rosidl", "xml" },
  callback = function()
    if vim.bo.filetype == "rosidl" then
      -- ROS IDL（.msg/.srv/.action）用 # 注释
      vim.opt_local.commentstring = "# %s"
      vim.opt_local.shiftwidth = 2
      vim.opt_local.tabstop = 2
    elseif vim.bo.filetype == "xml" then
      -- URDF/xacro/SDF 缩进 2 空格
      vim.opt_local.shiftwidth = 2
      vim.opt_local.tabstop = 2
    end
  end,
})

return {}

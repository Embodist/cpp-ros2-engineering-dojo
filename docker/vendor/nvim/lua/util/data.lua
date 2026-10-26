-- ============================================================================
--  数据文件工具（JSON / YAML / XML / TOML）
--    · format(sort_keys)  按文件类型调用 jq / yq / xmllint / taplo
--    · minify()           压缩（json / xml）
--    · validate()         语法校验（json / xml / yaml）
--
--  依赖 CLI：jq、yq(mikefarah)、xmllint(libxml2-utils)、taplo
--  被子命令（快捷键）调用，见 lua/config/keymaps.lua 的 <leader>j* 分组
-- ============================================================================

local M = {}

-- 取整个缓冲区内容
local function buf_text()
  return table.concat(vim.api.nvim_buf_get_lines(0, 0, -1, false), "\n")
end

local function set_buf(lines)
  vim.api.nvim_buf_set_lines(0, 0, -1, false, lines)
end

-- 执行外部过滤器；cmd 为 { "jq", "--indent", "2", "." }
-- 第二个参数作为 stdin 输入
local function run_filter(cmd, input)
  local out = vim.fn.system(cmd, input)
  if vim.v.shell_error ~= 0 then
    vim.notify("命令失败：" .. table.concat(cmd, " ") .. "\n" .. out, vim.log.levels.ERROR)
    return nil
  end
  -- 去掉尾部空行
  local lines = vim.split(out, "\n", { plain = true })
  while #lines > 0 and lines[#lines] == "" do
    table.remove(lines)
  end
  return lines
end

local function have(cmd)
  return vim.fn.executable(cmd) == 1
end

-- 返回当前文件类型对应的“格式化命令”
-- sort_keys = true 时对 JSON/YAML 按键名排序
local function build_format_cmd(ft, sort_keys)
  if ft == "json" or ft == "jsonc" then
    if not have("jq") then
      return nil, "需要 jq（apt install jq）"
    end
    local cmd = { "jq", "--indent", "2" }
    if sort_keys then
      table.insert(cmd, "--sort-keys")
    end
    table.insert(cmd, ".")
    return cmd
  elseif ft == "yaml" or ft == "yml" then
    if not have("yq") then
      return nil, "需要 yq（scripts/02-cli-tools.sh 会安装 mikefarah/yq）"
    end
    local cmd = { "yq", "-P", "--indent", "2" }
    table.insert(cmd, sort_keys and "sort_keys(..)" or ".")
    return cmd
  elseif ft == "xml" then
    if not have("xmllint") then
      return nil, "需要 xmllint（apt install libxml2-utils）"
    end
    return { "xmllint", "--format", "-" }
  elseif ft == "toml" then
    if not have("taplo") then
      return nil, "需要 taplo（:MasonInstall taplo 或 npm i -g @taplo/cli）"
    end
    return { "taplo", "fmt", "-" }
  end
  return nil, "当前文件类型不支持的格式化：" .. ft
end

--- 格式化当前缓冲区
--- @param sort_keys boolean|nil 是否按键名排序（JSON/YAML 有效）
function M.format(sort_keys)
  local ft = vim.bo.filetype
  local cmd, err = build_format_cmd(ft, sort_keys == true)
  if not cmd then
    vim.notify(err, vim.log.levels.WARN)
    return
  end
  local lines = run_filter(cmd, buf_text())
  if not lines then
    return
  end
  set_buf(lines)
  vim.notify("已格式化" .. (sort_keys and "（键已排序）" or ""), vim.log.levels.INFO)
end

--- 压缩为单行（json / xml）
function M.minify()
  local ft = vim.bo.filetype
  local cmd
  if ft == "json" or ft == "jsonc" then
    cmd = { "jq", "-c", "." }
  elseif ft == "xml" then
    cmd = { "xmllint", "--noblanks", "-" }
  else
    vim.notify("压缩仅支持 json / xml", vim.log.levels.WARN)
    return
  end
  if not have(cmd[1]) then
    vim.notify("需要 " .. cmd[1], vim.log.levels.WARN)
    return
  end
  local lines = run_filter(cmd, buf_text())
  if lines then
    set_buf(lines)
    vim.notify("已压缩", vim.log.levels.INFO)
  end
end

--- 语法校验（json / xml / yaml）
function M.validate()
  local ft = vim.bo.filetype
  local cmd
  if ft == "json" or ft == "jsonc" then
    cmd = { "jq", "empty", "." }
  elseif ft == "xml" then
    cmd = { "xmllint", "--noout", "-" }
  elseif ft == "yaml" or ft == "yml" then
    cmd = { "yq", "-e", "." }
  else
    vim.notify("校验仅支持 json / xml / yaml", vim.log.levels.WARN)
    return
  end
  if not have(cmd[1]) then
    vim.notify("需要 " .. cmd[1], vim.log.levels.WARN)
    return
  end
  local out = vim.fn.system(cmd, buf_text())
  if vim.v.shell_error == 0 then
    vim.notify("✔ 语法有效", vim.log.levels.INFO)
  else
    vim.notify("✘ 语法错误：\n" .. out, vim.log.levels.ERROR)
  end
end

return M

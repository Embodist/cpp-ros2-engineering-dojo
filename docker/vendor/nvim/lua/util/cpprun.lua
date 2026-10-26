-- ============================================================================
--  单文件 C/C++ 编译运行（刷题 / 实验 / 练习场）
--
--  为什么需要它：
--    CMake 工作流（cmake-tools）适合"工程"，但读开源片段、写练习、刷题时
--    更多是**一个 .cpp 文件**：想立刻编译+跑，看输出、看错误。
--
--  设计要点（都来自本机实测，不是猜的）：
--    · 库列表**不硬编码成败**：本机只有 libz.so.1（运行时），没有开发包符号链接
--      libz.so → `-lz` 会让链接直接失败。所以这里默认带上常用库，
--      一旦 ld 报 "cannot find -lX" 就**自动去掉该库重试**，并把"怎么装"提示出来。
--    · 输出文件默认就叫 `tmp.out`（放在源文件同目录，保证程序里相对路径能用）。
--    · 运行时用**浮动终端**（Snacks），跑完不关窗，方便看输出/继续交互。
--    · 只编译的那条会把 g++ 输出解析成 quickfix（配合 ]q / <leader>xx 跳错误）。
--
--  配置（可放在 nvim/lua/config/options.lua 或项目 .nvim.lua）：
--    vim.g.cpprun = {
--      flags   = { "-O2", "-g", "-Wall" },   -- 覆盖默认编译参数
--      libs    = { "pthread", "z" },         -- 覆盖默认库列表
--      out     = "tmp.out",                  -- 输出文件名
--      include = { ".", "include", "src" },  -- -I 目录
--    }
--
--  文档：docs/guide/03-keymaps.md（跳转/编译键位表）· docs/guide/04-cpp-workflow.md
-- ============================================================================

local M = {}

-- ── 默认参数（覆盖方式见文件头）────────────────────────────────────────────
local defaults = {
  compiler_cpp = "g++",
  compiler_c = "gcc",
  std_cpp = "c++20",
  std_c = "c17",
  flags = { "-O2", "-Wall", "-Wextra" },
  include = { ".", "include", "src" },
  -- 常用库：缺哪个自动跳过（本机 zlib 只有运行时库 → -lz 会被跳过并提示安装）
  libs = { "pthread", "dl", "m", "rt", "z" },
  out = "tmp.out",
}

-- 已知链接不上的库（会话级缓存，避免每次重试都白跑一遍）
local missing_libs = {}
-- 是否已探测过（库列表可能被 vim.g.cpprun 改，改了就重探）
local probed_signature = nil

-- ══════════════════════════════════════════════════════════════════════════
--  ⚠️ 本文件里的局部函数**必须按依赖顺序定义**（Lua 的 local 是词法作用域，
--     先调用后定义 → 运行时 "attempt to call global 'xxx' (a nil value)"）。
--     顺序（改文件时勿打破）：
--       defaults → merge_opts → lib_hint_impl → probe_lib → ensure_probed
--       → file_info → compile_cmd → extract_missing → term → M.*
--     这个错误在 `luac -p` / lua_ls 语法检查里看不出来，只有**真按一次键**才会暴露。
-- ══════════════════════════════════════════════════════════════════════════

local function merge_opts()
  local o = vim.deepcopy(defaults)
  local user = vim.g.cpprun
  if type(user) == "table" then
    for k, v in pairs(user) do
      o[k] = v
    end
  end
  return o
end

local function notify(msg, level)
  vim.notify("cpprun: " .. msg, level or vim.log.levels.INFO)
end

-- 常见缺失库 → 对应的安装命令（只做提示，不自动装）
local function lib_hint_impl(libs)
  local set = {}
  for _, l in ipairs(libs) do
    set[l] = true
  end
  if set.z then
    return "sudo apt install zlib1g-dev"
  elseif set.ssl or set.crypto then
    return "sudo apt install libssl-dev"
  elseif set.curl then
    return "sudo apt install libcurl4-openssl-dev"
  elseif set.fmt then
    return "sudo apt install libfmt-dev"
  end
  return nil
end

-- ── 可靠的库探测 ───────────────────────────────────────────────────────────
--  试验过的探针（都不可靠，别用）：
--    · gcc -print-file-name=libpthread.so  → 说“找不到”，但 -lpthread 明明能链（glibc 2.34+ 已合并）
--    · ldconfig -p | grep libX.so          → 只能证明“有运行时库”，证明不了“有开发包符号链接”
--  唯一可靠的办法：**真试链接一次**。用 -shared + 空翻译单元，不需要 main，开销 ~40ms/库，会话内只做一次。
local function probe_lib(compiler, lib)
  local r = vim.fn.system({ compiler, "-shared", "-x", "c++", "-o", "/dev/null", "/dev/null", "-l" .. lib })
  return vim.v.shell_error == 0
end

---@param o table merge_opts() 的结果
local function ensure_probed(o)
  local sig = table.concat({ o.compiler_cpp, o.compiler_c, table.concat(o.libs, ",") }, "|")
  if probed_signature == sig then
    return
  end
  probed_signature = sig
  local compiler = o.compiler_cpp
  local skipped = {}
  for _, lib in ipairs(o.libs) do
    if not probe_lib(compiler, lib) then
      missing_libs[lib] = true
      table.insert(skipped, lib)
    end
  end
  if #skipped > 0 then
    local hint = lib_hint_impl(skipped)
    notify(
      "本机链接不上："
        .. table.concat(skipped, ", ")
        .. "（已自动跳过，其余库照常）"
        .. (hint and ("；装开发包后自动启用：" .. hint) or ""),
      vim.log.levels.WARN
    )
  end
end

-- ── 当前文件信息 ───────────────────────────────────────────────────────────
local function file_info()
  local path = vim.api.nvim_buf_get_name(0)
  if path == "" then
    notify("当前缓冲区没有文件名（先 :write 保存）", vim.log.levels.WARN)
    return nil
  end
  local ft = vim.bo.filetype
  if not vim.tbl_contains({ "c", "cpp", "cu", "cxx", "objc", "objcpp" }, ft) then
    notify("当前文件不是 C/C++（filetype=" .. ft .. "）", vim.log.levels.WARN)
    return nil
  end
  local abs = vim.fn.fnamemodify(path, ":p")
  local dir = vim.fn.fnamemodify(abs, ":h")
  return {
    path = abs,
    dir = dir,
    name = vim.fn.fnamemodify(abs, ":t"),
    stem = vim.fn.fnamemodify(abs, ":t:r"),
    is_c = ft == "c",
  }
end

-- ── 组装编译命令 ───────────────────────────────────────────────────────────
---@param info table file_info() 的返回值
---@param skip table<string, boolean> 要跳过的库
local function compile_cmd(info, skip)
  local o = merge_opts()
  local parts = {}
  table.insert(parts, info.is_c and o.compiler_c or o.compiler_cpp)
  table.insert(parts, info.is_c and ("-std=" .. o.std_c) or ("-std=" .. o.std_cpp))
  vim.list_extend(parts, o.flags)
  for _, d in ipairs(o.include) do
    table.insert(parts, "-I" .. d)
  end
  table.insert(parts, vim.fn.shellescape(info.name))
  table.insert(parts, "-o")
  table.insert(parts, vim.fn.shellescape(o.out))
  for _, lib in ipairs(o.libs) do
    if not (skip and skip[lib]) then
      table.insert(parts, "-l" .. lib)
    end
  end
  return table.concat(parts, " ")
end

--- 从 ld 输出里解析出缺失的库（"cannot find -lz" → z）
local function extract_missing(output)
  local found = {}
  for lib in output:gmatch("cannot find %-l([%w_%+%-%.]+)") do
    found[lib] = true
  end
  return found
end

-- ── 浮动终端（Snacks）─────────────────────────────────────────────────────
--  无 UI（headless，如脚本/CI）时**不能**开终端：Snack 不可用 → 回退 `:terminal` → 会一直等输入而挂死。
--  这种情况改用同步 system() 并把输出打到 stdout，使它可以被脚本捕获。
local function no_ui()
  return #vim.api.nvim_list_uis() == 0
end

---@param cmd string 编译（+可选运行）的完整 shell 命令
local function term(cmd, cwd, title)
  if no_ui() then
    local out = vim.fn.system({ "sh", "-c", "cd " .. vim.fn.shellescape(cwd) .. " && " .. cmd .. " 2>&1" })
    io.stdout:write(out)
    return
  end
  local ok, Snacks = pcall(require, "snacks")
  if ok and Snacks and Snacks.terminal then
    Snacks.terminal(cmd, {
      cwd = cwd,
      auto_close = false, -- 跑完**不关窗**：要能看输出 / 继续输入
      auto_insert = true,
      start_insert = true,
      win = {
        position = "float",
        height = 0.8,
        width = 0.85,
        border = "rounded",
        title = " ▶ " .. title .. " ",
        title_pos = "center",
      },
    })
  else
    vim.cmd("botright split | terminal cd " .. vim.fn.shellescape(cwd) .. " && " .. cmd)
  end
end

-- ── 对外：编译并运行（<C-r>）──────────────────────────────────────────────
function M.run()
  local info = file_info()
  if not info then
    return
  end
  vim.cmd("silent! write") -- 先存盘：跑的是磁盘上的内容
  local o = merge_opts()
  ensure_probed(o) -- 探测库可用性（首次会提示哪些被跳过）
  local cmd = compile_cmd(info, missing_libs)
  -- term() 自己处理“有 UI → 浮窗 / 无 UI → 同步 system 并把输出打到 stdout”
  term(cmd .. " && ./" .. vim.fn.shellescape(o.out), info.dir, info.name)
end

-- ── 对外：只编译 → 错误进 quickfix（<leader>cc）────────────────────────────
function M.compile()
  local info = file_info()
  if not info then
    return
  end
  vim.cmd("silent! write")
  local o = merge_opts()
  ensure_probed(o)
  local out = vim.fn.system({ "sh", "-c", "cd " .. vim.fn.shellescape(info.dir) .. " && " .. compile_cmd(info, missing_libs) .. " 2>&1" })

  -- 自动跳过链接不上的库并重试一次（本机 zlib 就是这种情况）
  local missing = extract_missing(out)
  local n_missing = vim.tbl_count(missing)
  if n_missing > 0 then
    for lib in pairs(missing) do
      missing_libs[lib] = true
    end
    local retry = vim.fn.system({
      "sh",
      "-c",
      "cd " .. vim.fn.shellescape(info.dir) .. " && " .. compile_cmd(info, missing_libs) .. " 2>&1",
    })
    local names = vim.tbl_keys(missing)
    table.sort(names)
    local hint = lib_hint_impl(names)
    notify("链接库不可用，已自动去掉：" .. table.concat(names, ", ") .. (hint and ("（如需：" .. hint .. "）") or ""), vim.log.levels.WARN)
    out = retry
  end

  -- 解析 g++ 输出 → quickfix
  local old_efm = vim.o.errorformat
  vim.o.errorformat = table.concat({
    "%f:%l:%c: %trror: %m",
    "%f:%l:%c: %tarning: %m",
    "%f:%l:%c: %tote: %m",
    "%f:%l: %trror: %m",
    "%f:%l: %m",
    "%f:%l:%c: %m",
  }, ",")
  vim.fn.setqflist({}, " ", { title = "g++ " .. info.name, lines = vim.split(out, "\n"), efm = vim.o.errorformat })
  vim.o.errorformat = old_efm

  local items = vim.fn.getqflist()
  local errs = 0
  for _, it in ipairs(items) do
    -- 注意：%trror 解析出来的 type 是**小写 'e'**（Vim 的 qflist 约定），
    --      写成 "E" 会永远不匹配 → 误判为“编译通过”。这个错只有真按一次键才看得出来。
    if it.valid == 1 and it.type and it.type:lower() == "e" then
      errs = errs + 1
    end
  end
  if errs > 0 then
    vim.cmd("copen")
    notify(("%d 个编译错误 → quickfix（]q / [q 跳，<leader>xx 看详情）"):format(errs), vim.log.levels.ERROR)
  else
    notify(("编译通过 → %s/%s"):format(info.dir, o.out), vim.log.levels.INFO)
  end
end

-- ── 对外：只运行上次的产物（<leader>cC）───────────────────────────────────
function M.run_only()
  local info = file_info()
  if not info then
    return
  end
  local o = merge_opts()
  local bin = info.dir .. "/" .. o.out
  if vim.fn.executable(bin) ~= 1 then
    notify("找不到 " .. o.out .. "（先编译一次：<C-r> 或 <leader>cc）", vim.log.levels.WARN)
    return
  end
  term("./" .. vim.fn.shellescape(o.out), info.dir, info.name .. " (run only)")
end

-- ── 对外：显示当前编译命令（<leader>cR）───────────────────────────────────
function M.show_cmd()
  local info = file_info()
  if not info then
    return
  end
  local o = merge_opts()
  ensure_probed(o)
  local lines = {
    "# 当前文件的编译命令（cpprun）",
    "",
    "cd " .. info.dir,
    compile_cmd(info, missing_libs) .. " && ./" .. o.out,
    "",
    "# 说明",
    "#   · 输出文件：" .. o.out .. "（改：vim.g.cpprun.out）",
    "#   · 编译参数：vim.g.cpprun.flags",
    "#   · 链接库：vim.g.cpprun.libs（缺哪个会自动跳过并提示装 -dev 包）",
    "#   · 会话内已跳过的库：" .. (next(missing_libs) and table.concat(vim.tbl_keys(missing_libs), ", ") or "（无）"),
  }
  vim.cmd("botright new")
  local buf = vim.api.nvim_get_current_buf()
  vim.api.nvim_buf_set_lines(buf, 0, -1, false, lines)
  vim.bo[buf].filetype = "sh"
  vim.bo[buf].bufhidden = "wipe"
end

-- ── 对外：浮动 shell（<leader>fS）─────────────────────────────────────────
---@param dir? string 不传则用当前文件的目录（无文件名则用 cwd）
function M.shell(dir)
  local d = dir
  if not d then
    local name = vim.api.nvim_buf_get_name(0)
    d = name ~= "" and vim.fn.fnamemodify(name, ":p:h") or vim.fn.getcwd()
  end
  term(nil, d, "shell · " .. vim.fn.fnamemodify(d, ":t"))
  notify("当前目录：" .. d .. "（按 i 输入，<Esc><Esc> 退出输入）")
end

-- ── 自动挂载：只在 C/C++ 缓冲区里占 <C-r>（普通缓冲区保留内置 redo）────────
function M.attach(buf)
  buf = buf or vim.api.nvim_get_current_buf()
  local map = vim.keymap.set
  local opts = { buffer = buf, desc = "C++: 编译并运行 (tmp.out)" }
  map("n", "<C-r>", M.run, opts)
  map("n", "<leader>cc", M.compile, { buffer = buf, desc = "C++: 只编译 → quickfix" })
  map("n", "<leader>cC", M.run_only, { buffer = buf, desc = "C++: 只运行（不编译）" })
  map("n", "<leader>cR", M.show_cmd, { buffer = buf, desc = "C++: 查看编译命令" })
end

return M

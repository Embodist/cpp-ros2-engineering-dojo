-- ============================================================================
--  内存 / 性能 / 安全 分析工具封装
--  被 <leader>m* 键位调用（见 lua/config/keymaps.lua）
--  文档：docs/guide/19-memory-and-security.md
--
--  说明：这些工具都输出到 Snacks 终端，方便直接查看/复制。
--        真正的“定位”仍由你用调试器 + 修复完成，这里只负责跑得快、跑得顺。
-- ============================================================================

local M = {}

local function root()
  local ok, LazyVim = pcall(require, "lazyvim")
  if ok and type(LazyVim.root) == "function" then
    local ok2, r = pcall(LazyVim.root)
    if ok2 and type(r) == "string" and r ~= "" then
      return r
    end
  end
  return vim.uv.cwd()
end

local function has(exe)
  return vim.fn.executable(exe) == 1
end

local function missing(exe, pkg)
  vim.notify(("未找到 %s。安装：sudo apt install %s"):format(exe, pkg or exe), vim.log.levels.WARN)
end

-- 在 Snacks 终端中运行（可交互、可复制、可回看）
local function term(cmd)
  local c = type(cmd) == "table" and table.concat(cmd, " ") or cmd
  if Snacks and Snacks.terminal then
    Snacks.terminal(c, { cwd = root() })
  else
    vim.cmd("terminal " .. c)
  end
end

local function quote(s)
  return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function cmdline(argv)
  local parts = {}
  for _, v in ipairs(argv) do
    parts[#parts + 1] = quote(v)
  end
  return table.concat(parts, " ")
end

local function ask_exe()
  return vim.fn.input("可执行文件: ", root() .. "/build/", "file")
end

local function ask_args()
  return vim.split(vim.fn.input("程序参数: "), " ", { trimempty = true })
end

local function exe_argv()
  local e = ask_exe()
  if e == nil or e == "" then
    return nil
  end
  local argv = { e }
  vim.list_extend(argv, ask_args())
  return argv
end

local function has_compile_commands()
  local p = root() .. "/build/compile_commands.json"
  return vim.uv.fs_stat(p) ~= nil
end

-- ── Sanitizer 构建 ──────────────────────────────────────────────────────────
-- asan: 内存越界/泄漏/UAF | tsan: 数据竞争 | ubsan: 未定义行为 | msan: 未初始化读(需 clang)
function M.sanitizer_build(kind)
  local kinds = {
    asan = "-fsanitize=address,undefined",
    tsan = "-fsanitize=thread",
    ubsan = "-fsanitize=undefined",
    msan = "-fsanitize=memory",
  }
  if not kind then
    vim.ui.select({ "asan", "tsan", "ubsan", "msan" }, { prompt = "Sanitizer 构建类型" }, function(choice)
      if choice then
        M.sanitizer_build(choice)
      end
    end)
    return
  end
  local flags = kinds[kind]
  if not flags then
    vim.notify("未知 sanitizer：" .. tostring(kind), vim.log.levels.ERROR)
    return
  end
  local cflags = flags .. " -fno-omit-frame-pointer -g -O1"
  local dir = "build-" .. kind
  local cmd = table.concat({
    "cmake -S . -B " .. dir .. " -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1",
    "-DCMAKE_CXX_FLAGS=" .. quote(cflags),
    "-DCMAKE_C_FLAGS=" .. quote(cflags),
    "-DCMAKE_EXE_LINKER_FLAGS=" .. quote(flags),
    "&& cmake --build " .. dir .. " -j",
  }, " ")
  term(cmd)
end

-- ── valgrind 系列 ───────────────────────────────────────────────────────────
function M.memcheck()
  if not has("valgrind") then
    return missing("valgrind")
  end
  local argv = exe_argv()
  if not argv then
    return
  end
  term(
    "valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes "
      .. "--num-callers=30 --error-exitcode=1 "
      .. cmdline(argv)
  )
end

function M.massif()
  if not has("valgrind") then
    return missing("valgrind")
  end
  local argv = exe_argv()
  if not argv then
    return
  end
  term(
    "valgrind --tool=massif --massif-out-file=massif.out --stacks=yes "
      .. cmdline(argv)
      .. " ; echo '查看: ms_print massif.out | head -80   (或 massif-visualizer massif.out)'"
  )
end

function M.callgrind()
  if not has("valgrind") then
    return missing("valgrind")
  end
  local argv = exe_argv()
  if not argv then
    return
  end
  term(
    "valgrind --tool=callgrind --callgrind-out-file=callgrind.out "
      .. cmdline(argv)
      .. " ; echo '查看: callgrind_annotate callgrind.out | head -80   (或 kcachegrind callgrind.out)'"
  )
end

-- ── heaptrack 堆分析（定位分配热点/泄漏来源）────────────────────────────────
function M.heaptrack()
  if not has("heaptrack") then
    return missing("heaptrack")
  end
  local argv = exe_argv()
  if not argv then
    return
  end
  term(
    "heaptrack -o heaptrack.out "
      .. cmdline(argv)
      .. " ; echo '查看: heaptrack_print heaptrack.out | head -120   (或 heaptrack_gui heaptrack.out)'"
  )
end

-- ── perf 性能采样 ───────────────────────────────────────────────────────────
function M.perf()
  if not has("perf") then
    return missing("perf", "linux-perf")
  end
  local argv = exe_argv()
  if not argv then
    return
  end
  term(
    "perf record -g -F 999 --call-graph dwarf -o perf.data "
      .. cmdline(argv)
      .. " && perf report -i perf.data"
  )
end

-- ── 静态分析 ────────────────────────────────────────────────────────────────
function M.cppcheck()
  if not has("cppcheck") then
    return missing("cppcheck")
  end
  if has_compile_commands() then
    term(
      "cppcheck --project=build/compile_commands.json "
        .. "--enable=warning,performance,portability --inline-suppr "
        .. "--suppress=missingIncludeSystem --template=gcc --quiet 2>&1 | tee cppcheck.txt"
    )
  else
    term(
      "cppcheck --enable=warning,performance,portability --std=c++20 "
        .. "--inline-suppr --suppress=missingIncludeSystem --template=gcc "
        .. "-i build -i build-* . 2>&1 | tee cppcheck.txt"
    )
  end
end

function M.scan_build()
  if not has("scan-build") then
    return missing("scan-build", "clang-tools")
  end
  term(
    "scan-build -o scan-build-out cmake -S . -B build-scan -G Ninja "
      .. "-DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 "
      .. "-DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "
      .. "&& scan-build -o scan-build-out cmake --build build-scan"
  )
end

function M.flawfinder()
  if not has("flawfinder") then
    return missing("flawfinder")
  end
  local target = vim.uv.fs_stat(root() .. "/src") and "src include" or "."
  term("flawfinder --minlevel=4 --dataonly --context " .. target .. " 2>&1 | head -200")
end

function M.gitleaks()
  if not has("gitleaks") then
    return missing("gitleaks")
  end
  term("gitleaks detect --source . --redact -v")
end

-- ── 运行程序并把 sanitizer 输出载入 quickfix（best-effort）──────────────────
function M.sanitizer_quickfix()
  local argv = exe_argv()
  if not argv then
    return
  end
  local out = vim.fn.system(argv)
  vim.opt.errorformat = {
    "SUMMARY: %m %f:%l:%c", -- ASan: SUMMARY: AddressSanitizer: ... /path/f.cpp:12:3
    "%f:%l:%c: %m", -- UBSan: /path/f.cpp:12:3: runtime error: ...
    "%-G%.%#", -- 其余行忽略
  }
  vim.fn.setqflist({}, "r", { title = "Sanitizer", lines = vim.split(out, "\n") })
  vim.cmd("copen")
  vim.notify("已载入 sanitizer 输出到 quickfix（]q/[q 跳转）", vim.log.levels.INFO)
end

-- ── eBPF / 系统级追踪（bcc / bpftrace）───────────────────────────────────
-- Debian 的 bcc 工具命令带 `-bpfcc` 后缀
local function bcc(name)
  if has(name .. "-bpfcc") then
    return name .. "-bpfcc"
  end
  return name
end

-- memleak：追踪“正在运行”的进程里未释放的内存分配（无需重新编译，适合长期运行的 CAE/EDA 进程）
function M.memleak_bpf()
  local tool = bcc("memleak")
  if not has(tool) then
    return missing("memleak-bpfcc", "bpfcc-tools")
  end
  vim.ui.select({ "运行命令", "附加到 PID" }, { prompt = "bcc memleak" }, function(choice)
    if choice == "运行命令" then
      local argv = exe_argv()
      if not argv then
        return
      end
      term("sudo " .. tool .. " -c " .. quote(table.concat(argv, " ")))
    elseif choice == "附加到 PID" then
      local pid = vim.fn.input("PID: ")
      if pid == "" then
        return
      end
      term("sudo " .. tool .. " -p " .. pid .. " -a")
    end
  end)
end

-- bpftrace 常用预设（需 root）
function M.bpftrace()
  if not has("bpftrace") then
    return missing("bpftrace")
  end
  local presets = {
    ["系统调用计数（按进程）"] = "tracepoint:raw_syscalls:sys_enter { @[comm] = count(); }",
    ["打开的文件"] = 'tracepoint:syscalls:sys_enter_openat { printf("%s %s\\n", comm, str(args->filename)); }',
    ["read 返回字节数直方图"] = "tracepoint:syscalls:sys_exit_read /args->ret > 0/ { @bytes = hist(args->ret); }",
    ["malloc 调用栈 Top"] = "uprobe:/lib/x86_64-linux-gnu/libc.so.6:malloc { @[ustack] = count(); }",
  }
  vim.ui.select(vim.tbl_keys(presets), { prompt = "bpftrace 预设" }, function(choice)
    if not choice then
      return
    end
    term("sudo bpftrace -e " .. quote(presets[choice]))
  end)
end

-- semgrep：语义化安全/缺陷扫描（多语言，含 C++）
function M.semgrep()
  if not has("semgrep") then
    return missing("semgrep", "（pipx install semgrep）")
  end
  term("semgrep scan --config=auto --error . 2>&1 | tee semgrep.txt")
end

return M

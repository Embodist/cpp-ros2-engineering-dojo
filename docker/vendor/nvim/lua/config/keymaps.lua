-- ============================================================================
--  补充全局键位
--
--  ⚠️ LazyVim 已提供绝大多数主流键位（<leader>f* 查找 / <leader>s* 搜索 /
--     gd gr gI gy / <leader>ca 代码动作 / <leader>cr 重命名 / <leader>cf 格式化 /
--     <leader>gg lazygit / <leader>t* 测试 / <leader>d* 调试 ...）。
--     完整键位表见：docs/guide/03-keymaps.md
--
--  这里只补充 LazyVim 未覆盖、且不与既有键位冲突的映射。
--  加载顺序：lazyvim.config.keymaps -> 本文件（本文件优先）
-- ============================================================================

local map = vim.keymap.set

-- ── 终端 ──────────────────────────────────────────────────────────────────
-- 终端模式用 <Esc><Esc> 回到 Normal（比 <C-\><C-n> 好按）
map("t", "<Esc><Esc>", "<C-\\><C-n>", { desc = "Exit terminal mode" })

-- ── 保存 / 退出 ───────────────────────────────────────────────────────────
map("n", "<C-s>", "<cmd>write<cr>", { desc = "Save File" })
map("n", "<C-q>", "<cmd>quit<cr>", { desc = "Quit Window" })
map("i", "<C-s>", "<cmd>write<cr><esc>", { desc = "Save File" })
map("x", "<C-s>", "<esc><cmd>write<cr>", { desc = "Save File" })

map("n", "<leader>w", "<cmd>write<cr>", { desc = "Save File" })
map("n", "<leader>Q", "<cmd>qa!<cr>", { desc = "Quit All (force)" })

-- ── 主题（配色）实时切换 ───────────────────────────────────────────────
--    实现见 lua/config/theme.lua（列表 / 私货调整都在那里）；
--    另有 LazyVim 自带的 <leader>uC（选择器，滚动即预览）与 :Theme 命令。
local theme = require("config.theme")
map("n", "<leader>uM", function() theme.next(1) end, { desc = "主题: 下一个（实时）" })
map("n", "<leader>um", function() theme.next(-1) end, { desc = "主题: 上一个（实时）" })

-- ── 单文件 C/C++ 编译运行（<C-r>）─────────────────────────────────────────
--    实现见 lua/util/cpprun.lua；键位只在 C/C++ 缓冲区挂载（见 config/autocmds.lua），
--    所以普通缓冲区里的 <C-r>（Vim 内置 redo）不受影响。
--      <C-r>       编译并运行当前文件 → 浮动终端 + tmp.out
--      <leader>cc  只编译 → 错误进 quickfix（]q / [q 跳）
--      <leader>cC  只运行（不重编译）
--      <leader>cR  查看当前编译命令（含跳过的库）
local cpprun = require("util.cpprun")
map("n", "<leader>fS", function() cpprun.shell() end, { desc = "终端: 浮动 shell（当前文件目录）" })

-- ── 剪贴板 ────────────────────────────────────────────────────────────────
map("x", "<leader>y", [["+y]], { desc = "Yank to system clipboard" })
map({ "n", "x" }, "<leader>Y", [["+Y]], { desc = "Yank line to system clipboard" })

-- ── 重新加载配置（调试 Neovim 配置本身时很有用）───────────────────────────
map("n", "<leader>Rc", function()
  for name in pairs(package.loaded) do
    if name:match("^config") or name:match("^plugins") then
      package.loaded[name] = nil
    end
  end
  vim.cmd("source $MYVIMRC")
  vim.notify("Neovim 配置已重新加载", vim.log.levels.INFO)
end, { desc = "Reload Neovim Config" })

-- ── 注意：终端已由 LazyVim 提供 ────────────────────────────────────────────
--   <leader>ft  Terminal (Root Dir)
--   <C-/>       Terminal (toggle/focus)
-- 不要在此处再映射 <leader>t*，那是 Neotest 测试专用前缀（<leader>tt 等）。

-- ── 数据文件工具（JSON / YAML / XML / TOML）────────────────────────────────
--    实现见 lua/util/data.lua，依赖 jq / yq / xmllint / taplo
local data = require("util.data")
map("n", "<leader>jf", function() data.format(false) end, { desc = "数据: 格式化" })
map("n", "<leader>js", function() data.format(true) end, { desc = "数据: 格式化并按键名排序" })
map("n", "<leader>jm", function() data.minify() end, { desc = "数据: 压缩为单行" })
map("n", "<leader>jv", function() data.validate() end, { desc = "数据: 语法校验" })

-- ── 内存 / 性能 / 安全分析（<leader>m）────────────────────────────────────
--    实现见 lua/util/analyze.lua；工具都输出到 Snacks 终端
local an = require("util.analyze")
map("n", "<leader>ms", function() an.sanitizer_build() end, { desc = "Sanitizer 构建 (asan/tsan/ubsan/msan)" })
map("n", "<leader>mm", function() an.memcheck() end, { desc = "valgrind 内存检查 (memcheck)" })
map("n", "<leader>mg", function() an.massif() end, { desc = "valgrind 堆剖面 (massif)" })
map("n", "<leader>mc", function() an.callgrind() end, { desc = "valgrind 调用图 (callgrind)" })
map("n", "<leader>mh", function() an.heaptrack() end, { desc = "heaptrack 堆分析" })
map("n", "<leader>mp", function() an.perf() end, { desc = "perf 性能采样" })
map("n", "<leader>mx", function() an.cppcheck() end, { desc = "cppcheck 静态检查" })
map("n", "<leader>mb", function() an.scan_build() end, { desc = "clang 静态分析 (scan-build)" })
map("n", "<leader>mf", function() an.flawfinder() end, { desc = "flawfinder 安全审计" })
map("n", "<leader>mk", function() an.gitleaks() end, { desc = "gitleaks 密钥扫描" })
map("n", "<leader>my", function() an.semgrep() end, { desc = "semgrep 语义化安全扫描" })
map("n", "<leader>ml", function() an.memleak_bpf() end, { desc = "bcc memleak: 追踪运行中进程未释放分配" })
map("n", "<leader>mt", function() an.bpftrace() end, { desc = "bpftrace 系统追踪（预设）" })
map("n", "<leader>ma", function() an.sanitizer_quickfix() end, { desc = "运行并载入 sanitizer 输出到 quickfix" })

-- ── ROS 2 / 机器人（<leader>R）──────────────────────────────────────────────
--    实现见 lua/util/ros.lua；未装 colcon/ros2 时会给出提示
local ros = require("util.ros")
map("n", "<leader>Rb", function() ros.build() end, { desc = "ROS: colcon build (Debug+compile_commands)" })
map("n", "<leader>RB", function() ros.build_select() end, { desc = "ROS: colcon build 指定包" })
map("n", "<leader>Ru", function() ros.build_up_to() end, { desc = "ROS: colcon build --packages-up-to" })
map("n", "<leader>Rt", function() ros.test() end, { desc = "ROS: colcon test" })
map("n", "<leader>Rr", function() ros.run() end, { desc = "ROS: ros2 run" })
map("n", "<leader>Rl", function() ros.launch() end, { desc = "ROS: ros2 launch" })
map("n", "<leader>Rs", function() ros.shell() end, { desc = "ROS: 打开已 source 环境的 shell" })
map("n", "<leader>Rv", function() ros.topics() end, { desc = "ROS: ros2 topic list" })
map("n", "<leader>Rn", function() ros.nodes() end, { desc = "ROS: ros2 node list" })
map("n", "<leader>Re", function() ros.echo() end, { desc = "ROS: ros2 topic echo" })
map("n", "<leader>Rp", function() ros.packages() end, { desc = "ROS: 列出包" })
map("n", "<leader>Rw", function() ros.rviz() end, { desc = "ROS: rviz2" })
map("n", "<leader>Rk", function() ros.compile_commands() end, { desc = "ROS: 合并 compile_commands.json（供 clangd）" })

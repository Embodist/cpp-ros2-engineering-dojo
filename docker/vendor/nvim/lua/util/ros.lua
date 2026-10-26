-- ============================================================================
--  ROS 2 / 机器人 工作流封装（colcon / ros2 / rviz）
--  被 <leader>R* 键位调用（见 lua/config/keymaps.lua）
--  文档：docs/guide/23-robotics-embodied-ai.md
--
--  说明：colcon 不在 Debian 官方源；用 Docker(ros:jazzy) 或 pipx/conda 安装。
--        未安装时给出明确提示，而不是静默失败。
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

local function missing(exe, hint)
  vim.notify(("%s 未找到。%s"):format(exe, hint or ""), vim.log.levels.WARN)
end

local function term(cmd)
  local c = type(cmd) == "table" and table.concat(cmd, " ") or cmd
  Snacks.terminal(c, { cwd = root() })
end

local BUILD_ARGS = "--symlink-install --cmake-args -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"

function M.build()
  if not has("colcon") then
    return missing("colcon", "用 Docker(ros:jazzy) 或 pipx install colcon-common-extensions")
  end
  term("colcon build " .. BUILD_ARGS)
end

function M.build_select()
  if not has("colcon") then
    return missing("colcon")
  end
  local pkg = vim.fn.input("包名: ")
  if pkg == "" then
    return
  end
  term("colcon build --packages-select " .. pkg .. " " .. BUILD_ARGS)
end

function M.build_up_to()
  if not has("colcon") then
    return missing("colcon")
  end
  local pkg = vim.fn.input("构建到此包(含依赖): ")
  if pkg == "" then
    return
  end
  term("colcon build --packages-up-to " .. pkg .. " " .. BUILD_ARGS)
end

function M.test()
  if not has("colcon") then
    return missing("colcon")
  end
  term("colcon test --event-handlers console_direct+ ; colcon test-result --verbose")
end

function M.run()
  local pkg = vim.fn.input("包名: ")
  if pkg == "" then
    return
  end
  local exe = vim.fn.input("可执行文件: ")
  term(("ros2 run %s %s"):format(pkg, exe))
end

function M.launch()
  local pkg = vim.fn.input("包名: ")
  if pkg == "" then
    return
  end
  local file = vim.fn.input("launch 文件: ")
  term(("ros2 launch %s %s"):format(pkg, file))
end

-- 打开一个已 source 好 ROS/工作空间环境的 shell
function M.shell()
  term([[bash -lc 'source /opt/ros/*/setup.bash 2>/dev/null; source install/setup.bash 2>/dev/null; exec bash']])
end

function M.topics()
  term("ros2 topic list")
end

function M.nodes()
  term("ros2 node list")
end

function M.echo()
  local topic = vim.fn.input("topic: ")
  if topic == "" then
    return
  end
  term("ros2 topic echo " .. topic)
end

function M.packages()
  if has("colcon") then
    term("colcon list")
  else
    term("ros2 pkg list")
  end
end

function M.rviz()
  term("rviz2")
end

-- 合并 colcon 各包的 compile_commands.json 到工作空间根目录，供 clangd 使用
function M.compile_commands()
  if not has("jq") then
    return missing("jq")
  end
  term(
    [[sh -c 'jq -s "add" $(find build -maxdepth 3 -name compile_commands.json) > compile_commands.json && echo "merged -> $(jq length compile_commands.json) entries"']]
  )
  vim.notify("已生成根目录 compile_commands.json，重启 clangd 后生效（:LspRestart）", vim.log.levels.INFO)
end

return M

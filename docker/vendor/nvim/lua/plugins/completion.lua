-- ============================================================================
--  补全引擎：blink.cmp（LazyVim v16 默认）
--
--  ⚠️ 本文件存在的唯一原因（实测过的坑）：
--     blink.cmp 默认 `fuzzy.implementation = "prefer_rust_with_warning"`，会在初始化时
--     **从 GitHub 下载预编译的 Rust 模糊匹配二进制**（消息：`Downloading pre-built binary`）。
--     在没有外网/直连 GitHub 不通的环境里，这个下载会**一直挂住**，导致 blink 初始化
--     永远不完成 → **C++ 里完全没有补全菜单**（LSP 诊断、hover、跳转都正常，只有补全没有）。
--
--     解决办法就是把它锁成纯 Lua 实现（blink 官方支持，能力足够）：
--       fuzzy.implementation = "lua"   →  不下载、不联网、启动即可用
--
--  想换成更快的 Rust 匹配器（可选，需要联网或本机 Rust 工具链）：
--     方式 A（联网下载预编译二进制）：
--        把下面 implementation 改成 "prefer_rust_with_warning"，并在启动 Neovim 前
--        设置代理：HTTPS_PROXY=http://localhost:46000 nvim （blink 需要 fuzzy.proxy.from_env = true）
--     方式 B（本机编译，需 cargo）：
--        sudo apt install rustc cargo
--        :Lazy build blink.cmp          # 等价于 cargo build --release
--        然后 implementation = "prefer_rust"
--
--  常用键位（改后实测，见 docs/guide/03-keymaps.md §2.9）：
--    <C-Space>    手动打开补全菜单 / 再按一次看文档
--    <C-n> <C-p>  或 ↑ ↓   上下选择
--    <CR> 或 <C-y>        接受
--    **<Tab>**            **有补全菜单 → 直接取第一个**；无菜单 → 默认缩进（片段中则先跳占位符）
--    <C-e>                关闭菜单
--    <C-k>                签名帮助
--
--  Tab 的语义（本次按需求调整）：
--    ① 片段占位符激活中（cmp.snippet_active）→ 跳下一个占位符（不误接受补全）
--    ② 补全菜单/幽灵文本可见（cmp.is_visible）→ select_and_accept：**未导航时取第一个候选**，
--       已用 <C-n>/<C-p> 导航过则接受当前选中项
--    ③ 都不可见 → 返回 nil → 依次尝试 snippet_forward → fallback（即 Tab 的默认缩进行为）
--  为什么不用 blink 自带的 preset = "super-tab"：那个预设里 **没有 <CR>**，
--    直接切过去会把你现在的 <CR> 接受习惯干掉；这里只覆盖 <Tab>，其余保持 LazyVim 原样。
--
--  文档：docs/guide/05-clangd-lsp.md · docs/guide/15-troubleshooting.md
-- ============================================================================

return {
  {
    "saghen/blink.cmp",
    opts = {
      fuzzy = {
        -- 纯 Lua 匹配器：离线可用，不触发任何下载
        -- （blink 文档：implementation = 'lua' 时自动关闭预编译二进制下载）
        implementation = "lua",
      },
      completion = {
        -- 输入关键字就弹菜单（blink 默认即 true，这里写出来便于调）
        trigger = { show_on_keyword = true },
      },
      keymap = {
        -- 只覆盖 <Tab>，其余键位（<CR>/<C-y> 接受、<C-n>/<C-p> 选择、<C-e> 关闭）保持 LazyVim 原样
        ["<Tab>"] = {
          function(cmp)
            if cmp.snippet_active() then
              return cmp.accept()
            end
            if cmp.is_visible() then
              return cmp.select_and_accept()
            end
            return nil -- → snippet_forward → fallback（默认缩进）
          end,
          "snippet_forward",
          "fallback",
        },
      },
    },
  },
}

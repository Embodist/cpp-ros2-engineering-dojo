# Stage 1：基础机制 + 工程纪律

- 所有 60 道核心题独立定义在 `cards/`。序号 01～12 有 `include/dojo` 代码骨架与 `tests/` 可执行测试。
- 另有 5 个可选扩展在 `extensions/`（E01–E05），用于补模板/concepts、STL 深水区、C++20、内存模型、测试/CI；说明见 [extensions/README.md](extensions/README.md)。
- `stage1/manifest.json` 可由 `tools/train.py` 读取；`tracker.csv` 可给 Excel/文本编辑器使用。
- 01～12 的 stub 带有 `TODO`，**故意不实现**，以保证仓库可构建且初次测试失败。你可以删除 stub 然后完成。
- 13～60 和 E01～E05 在 `labs/<id>/` 自行建立代码和证据文件，验证依赖你记录的运行命令，不能被脚本自动认证。
- 覆盖强度与缺口见 [docs/CORE_GUIDELINES_MAP.md](../docs/CORE_GUIDELINES_MAP.md)。

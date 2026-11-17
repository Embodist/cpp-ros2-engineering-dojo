# 项目线证据（p01–p13）

工作区在 `stage1/projects/pXX`；参考实现已同步到 `stage1/projects/reference/`。这里只放**证据和复盘**。

- 项目 ↔ 先修题 ↔ Stage 2 映射：见 [stage1/PROJECT_TRACK.md](../../stage1/PROJECT_TRACK.md)。
- 每个项目一份 `pXX.md`，从 [_template.md](_template.md) 复制。
- 每条核查点必须有：测试名/命令、原始输出、环境；不能只写“已验证”。
- 生命周期/内存题跑 ASan/UBSan；并发题跑 TSan；性能题记录环境、规模、重复次数。
- 项目代码自己写；AI 只做 review、最小复现和边界提问。

推荐顺序（最小闭环）：

```text
p01 v2/v3 → p02a → p05 → p08 → p09 → p10 → p11 → p13
```

可选补齐：p02b、p03、p04、p06、p07、p12。

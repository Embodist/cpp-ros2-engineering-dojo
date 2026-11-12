# Stage 1 扩展任务（可选）

这 5 张卡用于补核心 60 题没有系统覆盖的缺口。它们**不改变 8 周主线**，按需选修。

| ID | 主题 | 补强 | 前置 | 建议时机 |
|---|---|---|---|---|
| E01 | 变参模板、SFINAE 与 concepts | T / Con | 03、38 | Level 2 |
| E02 | STL 深水区：list、rb_tree、traits、EBO | SL / T / 源码 | 03、29–36 | Level 2–4 |
| E03 | C++20 ranges 与 expected | SL / E / 现代 C++ | 03、07、35 | Level 3–4 |
| E04 | 并发内存模型 | CP | 08–12、45–52 | Level 3–4 |
| E05 | 测试、CI 与包管理 | SF / 工程 | 03 或 08 | Level 4–5 |

## 使用方式

```bash
python3 tools/train.py show E01
python3 tools/train.py lab E01
# 完成 labs/E01/README.md 和 evidence.md 后
python3 tools/train.py done E01 --manual
```

扩展任务与开放题一样，必须提交可运行测试/编译命令、实际输出、至少两个反例和未覆盖风险。

## 与核心题的关系

- 核心 60 题是主线，扩展是补丁；不要为了做扩展而跳过 01–12。
- E01 建议在 38 之后；E02 建议在 03/29–36 之后；E03 建议在 35 之后；E04 建议在 08–12/45–52 之后；E05 建议在完成一个组件之后。
- 覆盖强度的完整矩阵见 [Core Guidelines 覆盖矩阵](../../docs/CORE_GUIDELINES_MAP.md)。

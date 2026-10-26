---
name: json-tool
description: JSON 处理工具。校验 JSON 是否合法、格式化(美化)、提取指定路径的值。处理 JSON 数据、排查格式错误、从大 JSON 中取字段时使用。
---

# JSON Tool

使用脚本 `./scripts/json-tool.js` 处理 JSON。

## 用法

```bash
node ./scripts/json-tool.js validate '<json>'   # 校验合法性,输出 valid / 错误信息
node ./scripts/json-tool.js pretty '<json>'     # 美化输出
node ./scripts/json-tool.js get '<json>' 'a.b.c' # 按点路径取值,支持数组下标 a[0].b
```

JSON 作为单个参数传入(注意 shell 引号)。取不到路径时输出 `NOT_FOUND`。

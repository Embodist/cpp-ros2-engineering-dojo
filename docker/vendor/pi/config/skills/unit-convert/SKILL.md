---
name: unit-convert
description: 单位换算助手。支持温度(摄氏/华氏)、长度(m/km/mi)、重量(kg/lb)。需要做单位换算时使用。
---

# Unit Converter

使用脚本 `./scripts/convert.sh <value> <from> <to>` 做单位换算。

## 支持的换算

| from | to | 说明 |
|------|----|------|
| c | f | 摄氏 → 华氏 |
| f | c | 华氏 → 摄氏 |
| km | mi | 千米 → 英里 |
| mi | km | 英里 → 千米 |
| kg | lb | 千克 → 磅 |
| lb | kg | 磅 → 千克 |

## 用法

```bash
./scripts/convert.sh 100 c f    # → 212
./scripts/convert.sh 5 km mi
```

换算失败会输出 `ERROR: <原因>`。

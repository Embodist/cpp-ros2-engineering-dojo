# p01 学生管理系统 v1–v3

- 阶段：P1 语义与所有权
- 优先级：P0-1
- 代码位置：`stage1/projects/p01`
- 参考实现：`stage1/projects/reference/p01_student`
- 项目卡：`stage1/projects/p01/README.md`
- 功能要求：`stage1/projects/docs/功能要求清单.md`
- 先修 Stage 1：01, 02, 05, 07, 13, 14, 15, 16, 17, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36
- 项目依赖：-
- 推荐：-
- 对应 Stage 2：C01, C02

## 里程碑
- [ ] v1 值语义
- [ ] v2 所有权与多态
- [ ] v3 异常安全

## 能力连续
- 承接：值语义、RAII、const 正确性、异常安全
- 输出：所有权图、Repository 接口、异常保证与测试证据

## 工作流
1. 先做先修题：`python3 tools/train.py projects` 会显示缺口。
2. 在 `stage1/projects/p01` 写项目；AI 只做 review，不代写。
3. 先跑参考测试：`python3 tools/train.py project p01 --test`；再跑自己的测试、ASan/UBSan；并发题加 TSan；性能题记录环境。
4. 把命令、原始输出、反例写入 `notes/projects/p01.md`。
5. 完成：`python3 tools/train.py project p01 --done`。

## 完成标准
- 项目 README 与功能要求清单的验收项逐条有证据。
- `notes/projects/p01.md` 没有 TODO。

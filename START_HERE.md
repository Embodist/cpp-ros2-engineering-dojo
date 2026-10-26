# 开始训练：先跑一题，不要先看教程 3 小时

## 0. 环境
- 推荐 Ubuntu 24.04 / WSL2 Ubuntu / 原生 Linux。Windows 可通过 WSL2 使用。
- CMake >= 3.20、GCC >= 11 / Clang >= 14、Python3。使用 C++20；ROS2 版本以所安装发行版实际支持为准。
- `g++ --version && cmake --version && python3 --version`

## 1. 今天执行 01
```bash
python3 tools/train.py list
python3 tools/train.py build
python3 tools/train.py test 01        # 预期失败，记录第一条提示
sed -n '1,200p' stage1/cards/01-scope-exit.md
sed -n '1,200p' stage1/include/dojo/scope_exit.hpp
# 自己实现后：
python3 tools/train.py test 01
python3 tools/train.py done 01
```

## 2. 对每题严格按 6 个动作
1. **预测**：在笔记写出行为、不变量、边界和错误处理，不能先让 AI 给实现。
2. **编码**：只查标准库 API 文档，自己写核心算法；20～40 分钟卡住才寻求提示。
3. **验证**：跑当前可见测试，并自己新增至少 2 个反例；打开编译器警告。
4. **工具**：指针/生命周期题跑 ASan+UBSan；并发题单独用 TSan；性能题测量基准。
5. **评审**：让 AI 找缺陷但不直接改代码；你提交修复及设计解释。
6. **复现**：24～72 小时后新建空文件，脱离 AI 复写关键机制。

## 3. 难度梯度
- 01/02/04/05/07/10/11：可在 1～3 小时开始获得反馈；
- 03/06/08/09/12：包含隐藏边界或并发语义，建议切分子目标；
- Stage 2：真实组件可能需 2～7 天每个，严格以证据而非耗时判定完成。

## 4. 评分
见 [docs/SCORING.md](docs/SCORING.md)，每题 0～4 分 × 5 维，总分 20。建议 **>=16 且无致命正确性错误** 才算达到上线前代码评审标准。

## 5. 快速命令
```bash
python3 tools/train.py status
python3 tools/train.py show 03
python3 tools/train.py test 03
python3 tools/train.py test 03 --sanitize asan
python3 tools/train.py test 08 --sanitize tsan
python3 tools/train.py report
```

详情：`python3 tools/train.py --help`。TSan 在部分虚拟化/容器环境可能无法启动；失败要分清工具环境和代码问题。

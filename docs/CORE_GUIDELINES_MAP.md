# Core Guidelines × 训练题对应表

本文件是**学习索引**，原始规则应在官方版本核对。不要把训练要求误认为官方规范原文。

| 范围 | 需要熟练的原则 | 题目 | 机器人场景 |
|---|---|---|---|
| R（资源） | RAII, ownership, avoid raw new/delete | 01,02,03,04,21 | 传感器句柄、消息缓存、线程退出 |
| C（类） | Rule of Zero/Five, virtual destructor, copy/move | 01–05,21–28 | 节点组件、驱动层 |
| F / I（接口） | 值/引用/视图、前后置条件、依赖注入 | 05–07,37–44 | 传感器抽象、消息校验 |
| CP（并发） | lock lifetime、CV predicate、no callback under lock | 08,09,12,45–52 | Executor、回调组、背压 |
| E（错误） | exception safety、structured failures | 03,06,07,22,42 | 节点故障恢复、数据解析 |
| Per（性能） | measure before optimize | 03,10,11,31–36,56 | 高频数据通路/延迟抖动 |
| SF / SL / T | 模板、库、工程构建 | 03,05,29–36,38,53,54 | 消息类型/包管理 |

训练 60 题完整列表见 `stage1/cards/`，每张卡带有具体关联条款编码。

官方索引：https://github.com/isocpp/CppCoreGuidelines

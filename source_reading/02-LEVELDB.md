# LevelDB：5 天有目标的源码训练

仓库：https://github.com/google/leveldb

| 日 | 精读范围（路径以检出版本为准） | 证据 |
|---|---|---|
| 1 | `include/leveldb/db.h`, `options.h`, `status.h` | API 与错误/所有权契约，五个失败情境 |
| 2 | `db/db_impl.cc` 里的 Put/Write 相关路径 | `Put -> Write -> WAL -> MemTable` 调用链，附关键状态 |
| 3 | `db/memtable.*`, `db/skiplist.h`, `db/log_writer.*` | 跳表与日志的边界、同步策略；一个实验 |
| 4 | `table/` SSTable / iterator / filter 相关 | 查询可能经过哪些层；缓存与 IO 的取舍 |
| 5 | `db/db_test.cc` 等测试、性能测试 | 一个新增故障测试、一份基准、一次反事实预测 |

**别从首行一路读到末行**。每阶段先提问：谁拥有数据、何时持久化、崩溃后凭什么恢复、读如何看到一致状态？缺源码证据就标记待验证。

实际操作：
```bash
git clone https://github.com/google/leveldb.git
cd leveldb
git rev-parse HEAD
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLEVELDB_BUILD_TESTS=ON
cmake --build build -j2
ctest --test-dir build --output-on-failure
```
实际选项、测试可用性依当前仓库版本与构建依赖为准；这里没有把 LevelDB 源码附在 ZIP 里，也没有替你下载依赖。

与 ROS2 的关系：它训练系统代码阅读方法，而不是 DDS/Executor 的替代教材。时间紧时优先读 ROS2 源码。

# Stage 1 扩展攻略（E01–E05）

怎么用这份攻略：先补齐卡片标注的前置核心题，再运行 `python3 tools/train.py show E01` 读原卡、`python3 tools/train.py lab E01` 建目录。本攻略只给路线和最小片段，不代替卡片；完成后把命令、原始输出、至少两个反例和未覆盖风险写进 `stage1/labs/E0x/evidence.md`，再执行 `python3 tools/train.py done E0x --manual`。

## E01 变参模板、SFINAE 与 concepts

- 前置：核心 03、38 之后，建议 Level 2。
- 目标：为传感器记录做编译期约束工厂；用变参模板实现 `make_record(fields...)`，用折叠表达式实现 `print_fields(record)`，以 `static_assert` 检查字段数量与类型；SFINAE 与 concepts 各写一版并比较诊断。
- 核心概念：变参模板、折叠表达式、SFINAE（`enable_if`/`void_t`）、concept 与 `requires`、重载决议；负例必须编译失败，不用无约束兜底重载。
- 参考实现思路：① 定义字段 concept，覆盖算术、字符串和时间戳可转换；② 用 `requires (Field<Ts> && ...)` 约束 `make_record`；③ `print_fields` 用逗号折叠逐字段输出，并 `static_assert` 字段数；④ 写 SFINAE 版与 concepts 版对照；⑤ 编译 3 个正例和 3 个负例，保存诊断并解释选中了哪个重载。
- 关键代码片段：

```cpp
#include <concepts>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

template<class T> concept Field = std::integral<T> || std::floating_point<T> ||
  std::convertible_to<T, std::string>;

template<Field... Ts> struct SensorRecord { std::tuple<Ts...> fields; };

template<Field... Ts>
SensorRecord<Ts...> make_record(Ts... ts) {
  static_assert(sizeof...(Ts) == 3, "record needs exactly 3 fields");
  return {{std::move(ts)...}};
}
template<Field... Ts>
void print_fields(const SensorRecord<Ts...>& r) {
  std::apply([](const auto&... f) { ((std::cout << f << ' '), ...); }, r.fields);
}
```

- 验收证据：`stage1/labs/E01/README.md` 写接口契约、约束设计、SFINAE 与 concepts 取舍；`stage1/labs/E01/evidence.md` 记录编译命令（如 `g++ -std=c++20 -Wall -Wextra -Wpedantic stage1/labs/E01/main.cpp -o /tmp/e01`）、3 个正例运行输出、3 个负例完整报错（须指出违反的约束）和未覆盖条件；最后运行 `python3 tools/train.py done E01 --manual`。
- 常见坑：约束写宽导致错误输入走兜底重载；`requires` 位置错；空参数包折叠；只验证正例，不保存负例诊断。
- 与 ROS2 的关联：消息字段类型与数量适合编译期约束；取值范围、时间戳合法性和跨版本兼容只能运行期校验。约束过宽会把错误推迟到运行期。

## E02 STL 深水区：list、rb_tree、traits 与 EBO

- 前置：核心 03 与 29–36 之后，建议 Level 2–4。
- 目标：选 libstdc++ 或 libc++ 做源码对照；跟踪 `std::list` 插入/删除调用链、`std::map` 的 rb_tree 查找与不变量，用 traits 做编译期分派，解释 EBO，并对 vector/list/map 各做一组失效实验。
- 核心概念：节点式容器、红黑树不变量、迭代器分类、空基类优化、迭代器/引用/指针失效、源码版本可复查。
- 参考实现思路：① 固定实现版本，记录 commit SHA 与文件路径，不记易漂移行号；② 从 `push_back`/`erase` 追到节点分配与链接，画调用链；③ 列出 map 查找路径和 rb_tree 不变量表；④ 用 `iterator_category` 或自定义 trait 区分连续/非连续迭代器并重载分派；⑤ 三容器插入、删除后验证失效范围，记录一个预测错误。
- 关键代码片段：

```cpp
#include <iterator>
#include <list>
#include <vector>

template<class It> constexpr bool is_contiguous = std::contiguous_iterator<It>;
static_assert(is_contiguous<std::vector<int>::iterator>);
static_assert(!is_contiguous<std::list<int>::iterator>);

void invalidation() {
  std::vector<int> v{1, 2, 3};
  auto* p = v.data() + 1;
  v.reserve(8);              // 可能重分配：p 失效
  std::list<int> l{1, 2, 3};
  auto it = std::next(l.begin());
  l.erase(l.begin());        // 被删元素失效：it 仍有效
  (void)p;                   // 失效后不要解引用
}
```

- 验收证据：`stage1/labs/E02/README.md` 写结构选择、复杂度、失效规则与源码结论；`stage1/labs/E02/evidence.md` 记录源码路径/commit、实验命令（如 `g++ -std=c++20 -g -fsanitize=address,undefined ...`）、实际输出和失败注入；至少写出一个原先预测错误的点；最后运行 `python3 tools/train.py done E02 --manual`。
- 常见坑：以为 vector 每次插入都全失效（只有重分配/部分操作）；list 的 erase 只失效被删节点；map 只失效被删元素；凭记忆写行号。
- 与 ROS2 的关联：传感器历史缓存要在 vector、deque、list 与环形缓冲间选型；热路径上的节点分配和缓存局部性会直接影响延迟。

## E03 C++20 ranges 与 expected：传感器流水线

- 前置：核心 03、07、35 之后，建议 Level 3–4。
- 目标：实现传感器记录流水线；用 `std::ranges` 的 filter/transform/take 筛选、转换、截断；用 `std::expected` 返回结构化错误；覆盖空输入、时间戳非法、数值越界；与朴素循环对比。
- 核心概念：view 惰性求值与生命周期、悬垂 view、expected 的值/错误通道、chrono 测量、输出一致性。
- 参考实现思路：① 先写朴素版和测试；② 用 views 组合 filter/transform/take；③ 用 `std::expected`，工具链不支持时改写最小 `Expected<T,E>`；④ 三类错误各写一个测试；⑤ 比较复制次数与耗时，并记录编译器、标准版本和环境。
- 关键代码片段（假设工具链支持 `std::expected`，否则替换为最小 `Expected<T,E>`）：

```cpp
#include <expected>
#include <ranges>
#include <vector>

enum class Err { Empty, BadTimestamp, OutOfRange };
struct Sample { long long ts_ms; double value; };

std::expected<double, Err> scale(const Sample& s) {
  if (s.ts_ms < 0) return std::unexpected(Err::BadTimestamp);
  if (s.value < 0.0 || s.value > 100.0) return std::unexpected(Err::OutOfRange);
  return s.value * 2.0;
}

std::vector<double> first_values(const std::vector<Sample>& in) {
  auto v = in | std::views::filter([](const Sample& s) { return s.ts_ms >= 0; })
              | std::views::take(5)
              | std::views::transform([](const Sample& s) { return s.value; });
  return {v.begin(), v.end()};
}
```

- 验收证据：`stage1/labs/E03/README.md` 写 view 生命周期、错误建模和两种实现取舍；`stage1/labs/E03/evidence.md` 记录编译命令与标准版本、ranges/expected 支持情况、三类错误路径输出、与朴素实现的逐项一致性、chrono 测量环境与结果；最后运行 `python3 tools/train.py done E03 --manual`。
- 常见坑：view 引用局部临时对象或回调内数据，离开作用域后悬垂；用魔法值表示错误；只测正常路径；给出没有测量环境的性能结论。
- 与 ROS2 的关联：来自订阅回调的数据，其 view 不能跨回调保存；错误应视边界走返回值、异常或日志/状态，而不是混用。

## E04 并发内存模型：happens-before 与 memory_order

- 前置：核心 08–12、45–52 之后，建议 Level 3–4。
- 目标：实现一次性发布/订阅标志或序列锁；提供 relaxed、acquire/release、seq_cst 三个可切换版本；写压力测试验证发布后能看到完整数据；跑 TSan。
- 核心概念：happens-before、release/acquire 配对、relaxed 只保证原子性、数据竞争是 UB、TSan 能证明与不能证明的边界。
- 参考实现思路：① 三版本可切换；② 发布者先写 payload，再 store 标志；订阅者 load 后读 payload；③ 固定迭代次数并校验数据完整；④ 解释 relaxed 为何可能失败、acquire/release 如何建立 happens-before；⑤ 跑 TSan，记录环境，并设计一个失败或不可复现的对照实验。
- 关键代码片段：

```cpp
#include <atomic>

struct Channel {
  int payload = 0;
  std::atomic<bool> ready{false};
};

void publish(Channel& c, int v) {
  c.payload = v;  // 普通写
  c.ready.store(true, std::memory_order_release);
}

int consume(Channel& c) {
  while (!c.ready.load(std::memory_order_acquire)) {}
  return c.payload;  // 与 release 配对后可见
}
```

- 验收证据：`stage1/labs/E04/README.md` 写 happens-before 推导、内存序选择、何时该用 mutex；`stage1/labs/E04/evidence.md` 记录硬件/OS/编译器、迭代次数、TSan 输出（如 `g++ -std=c++20 -fsanitize=thread -g -O1 ...`）和失败对照；最后运行 `python3 tools/train.py done E04 --manual`。
- 常见坑：用 `volatile` 代替 atomic；relaxed 下读 payload 形成数据竞争；用 seq_cst 掩盖推导；把 TSan 通过当作正确性证明；忙等无退避。
- 与 ROS2 的关联：状态标志、回调交接、Executor 唤醒可评估原子或 mutex；换到弱内存模型架构时，未验证的顺序假设会失效。

## E05 测试、CI 与包管理

- 前置：完成 03 MiniVector 或 08 BoundedQueue 之一。
- 目标：把已完成核心题工程化；用 GoogleTest 或 Catch2 增加测试 target；加 CI 完成构建与测试；用 Conan、vcpkg 或 FetchContent 固定 tag/commit；保留原自测。
- 核心概念：target 分离、默认构建离线、依赖版本锁定、CI 可复现、生成物不入库。
- 参考实现思路：① 选 03 或 08 的源码与自测放入 `stage1/labs/E05`；② 新增测试 target，联网依赖只在显式开启测试时引入；③ CI 执行构建+测试；④ 记录依赖固定方式；⑤ 从全新构建目录复现，确认不依赖旧缓存。
- 关键代码片段（以 Catch2 为例，替换为你的 03/08 公开接口）：

```cpp
#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

TEST_CASE("clamp keeps values in range") {
  REQUIRE(clamp(-1, 0, 10) == 0);
  REQUIRE(clamp(5, 0, 10) == 5);
  REQUIRE(clamp(11, 0, 10) == 10);
}
```

- 验收证据：`stage1/labs/E05/README.md` 写构建/测试/依赖/CI 命令与取舍；`stage1/labs/E05/evidence.md` 记录干净构建日志、测试输出、依赖锁定方式和 CI 结果或本地模拟结果；可复现命令如 `cmake -S stage1/labs/E05 -B /tmp/e05-build -DCMAKE_BUILD_TYPE=Debug && cmake --build /tmp/e05-build && ctest --test-dir /tmp/e05-build --output-on-failure`，预期全部测试通过；最后运行 `python3 tools/train.py done E05 --manual`。
- 常见坑：默认构建就联网；FetchContent 跟随分支不固定；测试未注册到 CTest；提交 build 目录；为加测试破坏原自测。
- 与 ROS2 的关联：迁移到 ament_cmake + colcon 时，依赖与构建产物要在 CI 中明确缓存和版本固定，避免“本机能过、CI 不能过”。

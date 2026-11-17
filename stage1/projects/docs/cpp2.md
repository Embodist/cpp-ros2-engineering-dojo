# C++ 内功训练手册

> 适用对象：Python 转 C++，已写过指针 / 类 / 模板；应用方向：具身智能 / ROS2；语言标准：C++20。 核心原则：**代码你自己写，AI 只当教练**——给需求、挑错、分析、提优化方向，不代写实现。

## 0. 怎么使用这本手册

每个训练项目都是同一张「任务卡」，看到就知道要做什么：

1. **训练目标**：这个项目练哪几项内功
2. **必须实现**：要写出的类型、接口、功能（内部实现由你自己设计）
3. **核查点**：做完逐条打勾，每条都要有测试或工具输出作证据
4. **让 AI 分析什么**：写完后交给 AI 的具体任务
5. **优化与扩展**：通过后继续深挖的方向
6. **通过标准**：达到才进入下一个项目

### 标准工作流（每个项目都走一遍）

1. 读任务卡，用自己的话写出接口草案（只有声明，没有实现）
2. 先写测试（至少覆盖核查点），再写实现
3. 开 sanitizer 编译，测试全过，无警告
4. 逐条过核查点，没有证据的不打勾
5. 提交给 AI review（模板见附录 A）
6. 根据 review 优化，每个问题记入「错题本」（附录 C）
7. 写 200 字复盘：踩了什么坑，哪个概念现在才真懂

### 自己写的铁律

- 卡住先独立排查 30 分钟，再问 AI
- 问「为什么这里是 UB」「有哪些替代设计」「这段违反了哪条 Core Guideline」，不问「帮我写」
- AI 给了代码片段，读懂后关掉，自己默写一遍
- 每个项目隔几天后「从零重写核心部分」，写得更短更对才算真会
- 读标准库源码只读不抄

**节奏建议**：每周 8–10 小时，全部项目约 4–5 个月，可按阶段推进。

## 1. 环境与工程规范（一次搭好，全程复用）

### 工具链

- 编译器：GCC 13+ 或 Clang 16+（C++20 支持较完整，`std::format` 需要较新版本）
- 构建：CMake 3.20+；测试框架：doctest / Catch2 / GoogleTest 任选其一，全程只用一个
- 基准：Google Benchmark；调试：gdb / lldb；内存检查：ASan、UBSan、valgrind；并发检查：TSan
- 静态分析：clang-tidy（启用 `bugprone-*`、`cppcoreguidelines-*`、`modernize-*`、`performance-*`）；格式化：clang-format
- 在线对照：Compiler Explorer，用来看汇编、比较不同写法

### 推荐目录

```
cpp-training/
  CMakeLists.txt
  common/          公共测试夹具
  p01_student/     每个项目一个目录
  p02_smart_ptr/
  ...
  notes/           复盘与错题本
```

### CMake 要点（自己写出来）

- C++ 标准设为 20，开启 REQUIRED，关闭编译器扩展
- Debug 参数：`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -fsanitize=address,undefined -fno-omit-frame-pointer -g`
- 为 TSan 单独建一个构建配置（不能与 ASan 同时开）
- 每个项目一个测试可执行目标，`ctest` 一键全跑

### 公共测试夹具（放在 common/，几乎每个项目都用）

| 夹具 | 行为规格 |
| --- | --- |
| Tracer | 静态计数：默认构造 / 拷贝构造 / 移动构造 / 拷贝赋值 / 移动赋值 / 析构的次数与当前存活数；每个实例有唯一 id；提供计数清零接口 |
| ThrowOnN | 可配置「第 N 次拷贝或构造时抛异常」，用来扫描所有抛出点 |
| MoveOnly | 内部持有 `unique_ptr<int>`，不可拷贝，可移动 |
| NoexceptToggle | 用 bool 模板参数控制移动构造是否 noexcept，用来测试容器扩容是搬移还是拷贝 |
| LeakCheck | 测试前后检查 Tracer 存活数是否归零 |

## 2. Python → C++ 思维转换

| Python 习惯 | C++ 里要换成什么 |
| --- | --- |
| 变量是对象的名字（引用语义） | 变量就是对象本身（值语义），拷贝会真复制 |
| GC 自动回收 | RAII：析构函数确定性释放，生命周期由作用域和所有权决定 |
| `with` 管理资源 | 局部对象 + 析构函数，天然就是 `with` |
| 鸭子类型 | 模板 + concepts（编译期检查） |
| `list` / `dict` / `set` | `vector` / `unordered_map` 或 `map` / `unordered_set` |
| 切片 `a[1:3]` | `std::span`、`std::string_view`（只是视图，不拥有数据） |
| `None` | `std::optional`、非拥有指针、`variant` |
| 异常随便抛 | 要想清楚异常安全等级与 `noexcept` |
| GIL 保护 | 真并行，数据竞争是 UB，必须自己同步 |
| 越界抛 IndexError | `operator[]` 越界是 UB，不会报错 |

### 十个最常见的坑（每个都自己写代码复现一次，再修好）

1. 悬空引用：返回局部变量的引用
2. 迭代器 / 指针失效：`vector` 扩容后继续用旧迭代器
3. 对象切片：按值把派生类存进基类容器
4. 基类析构函数不是 virtual
5. 未初始化变量、有符号整数溢出、数组越界（UB）
6. 悬空视图：`string_view` / `span` 指向已销毁的临时对象
7. 隐式转换与窄化：`int` 与 `size_t` 比较、`unsigned` 减法回绕
8. `shared_ptr` 循环引用泄漏
9. `const` 正确性缺失：该是 `const&` 的写成了拷贝或非 const
10. 头文件与 ODR：重复定义、忘记 `inline`、静态初始化顺序

## 3. 能力地图与总路线

| 阶段 | 项目 | 主要训练的内功 |
| --- | --- | --- |
| P1 语义与所有权 | 项目 1 学生管理系统（v1–v3） | 值语义、传参、所有权、多态、异常安全 |
| P2 资源管理 | 项目 2 UniquePtr / SharedPtr，项目 3 String | RAII、五法则、移动语义、EBO、控制块、SSO |
| P3 容器与泛型 | 项目 4 List，项目 5 Vector，项目 6 Arena 分配器 | 模板、迭代器、concepts、对象生命周期、分配器 |
| P4 抽象与并发 | 项目 7 Function，项目 8 线程安全队列与线程池，项目 9 SPSC 无锁队列 | 类型擦除、`jthread`、内存序、false sharing |
| P5 机器人方向综合 | 项目 10 迷你发布订阅与执行器，项目 11 零分配实时控制环，项目 12 小型数学库 | 回调与生命周期、实时约束、数值与内存布局 |

阶段末写一页「能力自评」（附录 C），不达标的项目回头重做。

## 项目 1：学生管理系统（三版进化）

**训练目标**：值语义与传参、`vector` + 算法、所有权设计、多态与虚析构、`shared_ptr` / `weak_ptr`、异常安全。

### v1 值语义版

**必须实现**

```cpp
struct Student {
    int id;
    std::string name;
    int age;
    std::map<std::string, double> scores;   // 科目 -> 成绩
    auto operator<=>(const Student&) const = default;
};
```

- 仓库类提供：`add` / `remove` / `update`、按 id 和按姓名查找、按任意字段排序（`std::ranges::sort` + 投影）、某科平均分、某科前 N 名、`save` / `load`
- 约束：id 唯一；姓名非空；成绩 0–100；文件格式自选（CSV 或二进制）
- 所有查询函数 `const`；每个函数参数的传递方式（值 / `const&` / `&&` / `string_view`）都要在注释里写一句理由
- `find` 的返回类型自己选（指针 / `optional<T>` / `optional<reference_wrapper<const T>>`），写出取舍理由
- 文件读写用 fstream 的 RAII，不手动 close；失败要有明确错误信息

**核查点**

- ☐ 查询函数全部 const 正确
- ☐ 找不到时不返回悬空指针或迭代器
- ☐ 用测试复现并解释：remove 之后，之前拿到的迭代器 / 指针为什么失效
- ☐ 用 Tracer 证明排序、查询没有多余的 Student 拷贝
- ☐ ASan / UBSan 干净
- ☐ 加载损坏文件（截断 / 乱码 / 非法成绩）不崩溃，也不留下半成品状态

### v2 所有权与多态版

**新增需求**

- 阶段 a：`Person` 抽象基类，派生 `Student`、`Teacher`；总表用 `std::vector<std::unique_ptr<Person>>` 存储
- 阶段 b：新增 `Course`，学生与课程多对多。总表改为持有 `shared_ptr<Student>`；**先故意让双方都用 `shared_ptr` 互相持有**，复现循环引用泄漏，再改成 Course 只持有 `weak_ptr<Student>`
- 实现多态拷贝 `clone()`（虚函数，返回 `unique_ptr<Person>`）

**核查点**

- ☐ `Person` 有虚析构；用析构计数证明：去掉 virtual 后派生类析构不会被调用
- ☐ 复现对象切片，并说明现象
- ☐ 循环引用：析构计数证明泄漏；改用 `weak_ptr` 后析构计数归零
- ☐ 删除学生后，Course 里的 `weak_ptr::lock()` 返回空，不会访问已销毁对象
- ☐ 能说清为什么多态拷贝不能用拷贝构造，要用 `clone()`

### v3 异常安全版

**新增需求**

- 批量导入成绩：任何一条非法或抛异常，整个系统状态保持导入前（强异常保证）
- 自定义异常层次：`ValidationError`、`IoError`
- 导出：先写临时文件，成功后再原子重命名

**核查点**

- ☐ 用 ThrowOnN 在导入流程的每个抛出点注入异常，系统状态与导入前逐字段相等
- ☐ 标出每个函数属于基本保证 / 强保证 / 不抛保证中的哪一级
- ☐ 实现一次 copy-and-swap，并说明它的代价与适用场景
- ☐ 手写简易 `ScopeGuard`（析构时执行回调，可 `dismiss()`），用在导出流程里

### 让 AI 分析什么

- 「逐个函数检查参数传递方式是否合理，指出可以改成 `string_view` / `span` / 移动的地方，并说明风险」
- 「找出这段代码里所有可能的迭代器 / 引用失效场景」
- 「评审所有权设计：哪些指针应该是拥有的、哪些是观察的？有没有更简单的设计？」
- 「检查异常安全：每个函数属于哪个保证等级，哪些地方抛异常会泄漏或留下不一致状态」

### 优化与扩展

- 用 `std::format` 做输出；用 `std::span` 做只读接口；用 concepts 约束通用查询函数
- 把存储换成 `unordered_map<int, Student>`，观察接口与失效规则的变化
- 项目 3、5 完成后，回头把标准容器替换成自己的 String 和 Vector

**通过标准**：v3 所有核查点都有测试证据；不看代码能口述每个类的所有权关系。

## 项目 2：UniquePtr / SharedPtr / WeakPtr

**训练目标**：RAII、独占与共享所有权、五法则、move-only 类型、空基类优化、控制块与引用计数、原子操作、自定义 deleter。

### 2a UniquePtr

**必须实现**

```cpp
template<class T, class D = DefaultDelete<T>>
class UniquePtr {
public:
    UniquePtr() noexcept;
    explicit UniquePtr(T* p) noexcept;
    UniquePtr(T* p, D d) noexcept;
    ~UniquePtr();
    UniquePtr(UniquePtr&&) noexcept;
    UniquePtr& operator=(UniquePtr&&) noexcept;
    UniquePtr(const UniquePtr&) = delete;
    T* get() const noexcept;
    T* release() noexcept;
    void reset(T* p = nullptr) noexcept;
    void swap(UniquePtr&) noexcept;
    T& operator*() const;
    T* operator->() const noexcept;
    explicit operator bool() const noexcept;
};
template<class T, class... Args> UniquePtr<T> MakeUnique(Args&&... args);
```

- 自己写 `DefaultDelete<T>`；deleter 用 `[[no_unique_address]]` 存储，使 `sizeof(UniquePtr<int>) == sizeof(int*)`
- 数组特化 `UniquePtr<T[]>`（带 `operator[]`）
- 派生类到基类的转换构造，用 concepts（`std::convertible_to`）约束

**核查点**

- ☐ `static_assert` 证明不可拷贝；移动构造 `is_nothrow_move_constructible`
- ☐ 移动后源对象为空；自移动赋值 `p = std::move(p)` 安全
- ☐ 自定义 deleter 能管理 `FILE*`（`fclose`），也能用带状态的 lambda
- ☐ `sizeof` 断言通过
- ☐ 与 `std::unique_ptr` 对同一组操作做差分测试
- ☐ Tracer + ASan 证明无泄漏、无重复释放

### 2b SharedPtr / WeakPtr

**必须实现**

- 控制块：强引用计数、弱引用计数、deleter；「销毁对象」与「释放控制块」时机分离
- `SharedPtr`：拷贝 / 移动、`use_count`、`reset`、`swap`、`operator bool`
- `WeakPtr`：`lock()`、`expired()`、`use_count()`
- `MakeShared<T>(args...)`：一次分配同时放下控制块和对象
- 计数用 `std::atomic`，为每个操作选择并说明 memory order
- 选做：`enable_shared_from_this`、aliasing 构造

**核查点**

- ☐ 拷贝 +1、析构 -1；最后一个强引用释放时对象析构；弱引用不阻止对象析构，但阻止控制块释放
- ☐ 循环引用测试：互持 SharedPtr 泄漏，改一方为 WeakPtr 后正常释放
- ☐ 多线程拷贝 / 销毁同一对象的多个副本，TSan 无报告（注意：被指向的对象本身不是线程安全的）
- ☐ `MakeShared` 比 `SharedPtr(new T)` 少一次分配（重载 `operator new` 计数验证）
- ☐ 自定义 deleter 的类型不出现在 `SharedPtr<T>` 的类型里，能说清这是类型擦除

### 让 AI 分析什么

- 「检查我的移动赋值和自赋值有没有异常安全问题」
- 「审查控制块的 atomic 操作：我选的 memory order 是否足够？是否过强？说明理由」
- 「检查有没有重复释放、对象与控制块释放时序的漏洞」
- 「我的实现和 libstdc++ 的实现有什么本质差异？」（贴出你读过的那一段）

### 优化与扩展

- 对比 `seq_cst` 与 acquire / release 的 benchmark 差异
- 实现 `intrusive_ptr` 并对比开销；思考为什么机器人系统里消息常用共享所有权传递

**通过标准**：所有核查点有证据；能口述 `shared_ptr` 的控制块布局与释放时序。

## 项目 3：String（含小字符串优化）

**训练目标**：五法则、深拷贝与移动、SSO（小字符串优化）、别名安全、与 `string_view` 互操作。

**必须实现**

- 构造：默认、`const char*`、`(n, ch)`、`std::string_view`（explicit）；拷贝 / 移动；析构
- 修改：`push_back`、`append`、`operator+=`、`insert`、`erase`、`clear`、`reserve`、`resize`
- 访问：`operator[]`、`at`、`front` / `back`、`data`、`c_str`（始终以 NUL 字符结尾）、`size`、`capacity`
- 查找与切片：`find`（字符 / 子串）、`substr`、`starts_with` / `ends_with`
- 比较与转换：`==`、`<=>`、`operator+`、`operator std::string_view`
- SSO：短串（如不超过 15 字节）存放在对象内部，不做堆分配；长串走堆；布局与模式标记自己设计

**核查点**

- ☐ `sizeof(String)` 合理，并能画出两种模式的内存布局
- ☐ 短串路径零堆分配（重载全局 `operator new` 计数证明）
- ☐ 移动构造 `noexcept`；长串移动不分配；能解释短串移动为何实质是拷贝
- ☐ 别名安全：`s += s`、`s.append(s.c_str())`、`s.insert(0, s)`
- ☐ 含 NUL 字符的串，长度与内容都正确
- ☐ 分配失败时原对象不变（强异常保证）
- ☐ 与 `std::string` 做随机操作差分测试

**让 AI 分析什么**

- 「找出所有由别名引起的 UB 场景」
- 「SSO 用联合体时，对齐与严格别名（type punning）有没有问题」
- 「对比 libstdc++ 与 libc++ 的 SSO 布局，各自的取舍是什么」

**优化与扩展**

- 研究 COW（写时复制）为什么在 C++11 之后被淘汰
- 调整增长因子与 SSO 阈值，用 benchmark 对比 `std::string`
- 与 `std::pmr::string` 对比分配行为

**通过标准**：所有核查点有证据；差分测试 10 万步无差异。

## 项目 4：List\<T>（双向链表）

**训练目标**：节点所有权、自定义迭代器、迭代器失效规则、哨兵节点、异常安全、concepts。

**必须实现**

- `List<T, Alloc>`：双向循环链表，使用哨兵节点
- 操作：`push_front` / `push_back`、`emplace_front` / `emplace_back`、`pop_front` / `pop_back`、`insert(pos, ...)`、`erase(pos)`、`clear`、`size`（O(1)）
- 算法：`splice`（整表 / 单节点 / 区间）、`reverse`、`sort`（稳定归并，O(n log n)）、`merge`、`unique`、`remove_if`
- 迭代器：`iterator` / `const_iterator` / `reverse_iterator`；`iterator` 可隐式转成 `const_iterator`
- 拷贝 / 移动 / `initializer_list` 构造

**核查点**

- ☐ `static_assert(std::bidirectional_iterator<List<int>::iterator>)`
- ☐ 节点用 `allocator_traits` 分配；元素构造失败时释放节点，不泄漏
- ☐ 能说明哨兵节点如何省掉空表的特判
- ☐ 失效规则有测试：`erase` 只让被删迭代器失效；`splice` 后迭代器仍有效，且属于新链表
- ☐ 先用 `unique_ptr` 链式节点实现，复现 10 万节点析构时栈溢出，再改为迭代销毁
- ☐ `sort` 稳定且零元素拷贝（Tracer 证明）
- ☐ 支持 `MoveOnly` 元素
- ☐ 与 `std::list` 差分测试

**让 AI 分析什么**

- 「迭代器类别是否名副其实，`const_iterator` 转换是否正确」
- 「`splice` / `sort` 的异常安全与失效保证」
- 「模板代码哪些地方可以用 concepts 简化」

**优化与扩展**

- 节点池分配（接项目 6）
- 遍历性能对比 `vector`，用 benchmark 解释缓存局部性
- 单向 `ForwardList` 版本

**通过标准**：所有核查点有证据。

## 项目 5：Vector\<T, Alloc>（旗舰项目）

前置：项目 1–4。原则：**先天真，再重构，再加固**。

### 五个版本

| 版本 | 内容 | 重点 |
| --- | --- | --- |
| v0 | 天真版：`new T[]`，T 必须可默认构造；实现 `push_back`、`reserve`、拷贝 / 移动 | 先跑起来，用 Tracer 记录多余的构造与拷贝 |
| v1 | 重构：原始内存 + `construct_at` / `destroy_at`；三指针；五法则 | 内存分配与对象生命周期分离 |
| v2 | 强异常保证：`move_if_noexcept`、扩容守卫、别名安全 | 异常安全与 noexcept |
| v3 | `insert` / `emplace` / `erase` / `assign` / 区间构造 | 重叠搬移、迭代器类别分发 |
| v4 | C++20 打磨：concepts、`<=>`、constexpr、`erase_if`、ranges、`span` | 现代接口 |

### 必须实现的接口

- 构造 / 赋值：默认、`(n)`、`(n, v)`、`initializer_list`、迭代器区间、拷贝、移动
- 容量：`size`、`capacity`、`empty`、`reserve`、`shrink_to_fit`、`resize`、`max_size`
- 访问：`[]`、`at`、`front`、`back`、`data`
- 迭代器：`begin` / `end`、`cbegin` / `cend`、`rbegin` / `rend`；自定义迭代器类满足 `std::contiguous_iterator`
- 修改：`push_back`、`emplace_back`、`pop_back`、`insert`、`emplace`、`erase`、`clear`、`assign`、`swap`
- 非成员：`==`、`<=>`、`swap`、`erase`、`erase_if`

### 关键设计要求

- 底层存储不用 `unique_ptr<T[]>`（它会默认构造 capacity 个元素）；智能指针用在扩容时新缓冲区的 RAII 守卫
- 增长因子取 1.5 或 2，并写出取舍理由
- 扩容：能 noexcept 移动就移动，否则拷贝（`move_if_noexcept` 思路）
- `emplace_back` 完美转发；`push_back(v[0])` 扩容时不能悬空
- 移动构造 `noexcept`；移动后源对象为空且可用
- 区间 `insert` 按迭代器类别分支：input 迭代器逐个插入，forward 迭代器先算距离

### 核查点

**资源**

- ☐ Tracer 构造数等于析构数；ASan / UBSan 干净
- ☐ 不对未构造的内存做赋值或读取

**语义**

- ☐ `push_back(v[0])`、`insert(pos, v[i])` 在扩容与不扩容两种情况下都正确
- ☐ 自赋值、自 swap 安全
- ☐ `clear` / `resize` 缩小不释放 capacity；`reserve` 不会缩小
- ☐ 空 vector：`begin() == end()`

**异常**

- ☐ 用 ThrowOnN 扫描 `push_back` / `insert` / 拷贝构造 / 赋值的每个抛出点：原数据不变（强保证），且不泄漏
- ☐ `Vector<Vector<int>>` 扩容走移动（用 NoexceptToggle 对比验证）

**类型**

- ☐ `Vector<unique_ptr<int>>`、`Vector<shared_ptr<T>>` 行为正确，`use_count` 符合预期
- ☐ 迭代器通过 `contiguous_iterator` 断言；不满足约束时，报错指向 concept

**复杂度**

- ☐ `push_back` 摊还 O(1)：n 次插入的总拷贝 / 移动次数不超过 c·n
- ☐ 差分测试：与 `std::vector` 随机操作 10 万步无差异

### 让 AI 分析什么

- 「逐个抛出点分析我的异常安全保证等级」
- 「找出所有迭代器 / 引用失效点，并对照标准要求」
- 「`insert` 的重叠搬移是否正确，边界（pos 等于 end、n 为 0）是否覆盖」
- 「对比 libstdc++ 的扩容插入实现，我缺了什么」

### 优化与扩展

- 用 `std::bit_ceil` 计算容量；用 `[[no_unique_address]]` 压缩分配器占用
- 对 trivially copyable 类型走 `memcpy` / `memmove` 快路径（`if constexpr` + `is_trivially_copyable_v`），用 benchmark 对比
- 实现分配器传播（POCCA / POCMA / POCS）
- 实现 `constexpr` Vector，用 `static_assert` 测试

**通过标准**：全部核查点有证据；能不看资料手写 v1 核心（扩容 + 析构）并讲清每个决策。

## 项目 6：Arena 与对象池分配器

**训练目标**：分配器模型、对齐、对象池、与容器配合；这是机器人控制环里「运行期不 malloc」的前置技能。

**必须实现**

- `ArenaResource`：线性（bump）分配，`allocate(size, align)`、`reset()`，不支持单个释放
- `PoolAllocator<T>`：固定大小块 + 空闲链表，满足 Allocator 要求（`value_type`、`allocate`、`deallocate`、相等比较）
- 与你的 `Vector<T, PoolAllocator<T>>`、`List<T, PoolAllocator<T>>` 配合使用
- 选做：继承 `std::pmr::memory_resource`（`do_allocate` / `do_deallocate` / `do_is_equal`），与 `std::pmr::vector` 联用

**核查点**

- ☐ 对齐正确：`max_align_t` 与超对齐类型（`alignas(64)`）都对
- ☐ 池满行为明确（抛异常或返回失败）并有测试
- ☐ 零 malloc：重载全局 `operator new` 计数，运行阶段为 0
- ☐ 整数溢出检查：`n * sizeof(T)` 溢出时安全失败
- ☐ benchmark：大量小对象分配 / 释放，对比 `std::allocator`
- ☐ ASan 无越界；选做：用 ASan 的 poison 接口标记已释放块

**让 AI 分析什么**

- 「对齐计算与指针算术有没有 UB」
- 「空闲链表在什么操作序列下会被破坏」
- 「线程安全方案的取舍：每线程 arena、加锁、无锁」

**优化与扩展**

- 多种块大小的分级池；线程局部池
- 对比 `std::pmr::monotonic_buffer_resource`

**通过标准**：核查点有证据；Vector / List 接入后差分测试仍通过。

## 项目 7：Function\<R(Args...)>（类型擦除）

**训练目标**：类型擦除、小缓冲优化（SBO）、可调用对象的生命周期、`std::invoke`。

**必须实现**

- `Function<R(Args...)>`：可存 lambda、函数指针、有状态仿函数；拷贝 / 移动；空状态；`operator bool`；`operator()`（空调用抛 `bad_function_call`）
- `MoveOnlyFunction<R(Args...)>`：能存 move-only 的 callable（例如捕获 `unique_ptr` 的 lambda）
- SBO：小 callable 存在对象内部缓冲，大的走堆，阈值自己定
- 选做：`target<T>()`

**核查点**

- ☐ 小 callable 零堆分配；大 callable 恰好一次分配
- ☐ 存入有状态 callable 后，拷贝 / 移动次数符合预期（Tracer）
- ☐ 空调用抛异常；移动后源对象为空
- ☐ 捕获引用的 lambda 悬空场景能说明，并写测试复现
- ☐ 移动 / 交换 `noexcept`；拷贝构造抛异常时无泄漏
- ☐ 能说清类型擦除的两种实现：虚函数基类 vs 手写函数指针表，各自的取舍

**让 AI 分析什么**

- 「SBO 缓冲的对齐与大小判断有没有 UB」
- 「拷贝与销毁路径的异常安全」
- 「与 `std::function` 的差异，标准为什么要新增 `move_only_function`」

**优化与扩展**

- 对比虚函数版与函数指针表版的调用开销
- 把它作为项目 8 线程池的任务类型

**通过标准**：核查点有证据；能口述类型擦除的完整数据流。

## 项目 8：线程安全队列与线程池

**训练目标**：`std::jthread` / `stop_token`、mutex + condition\_variable、虚假唤醒、关闭语义、异常传播、`future` / `packaged_task`。

**必须实现**

- `BlockingQueue<T>`：有界与无界两种；`push`、`pop`（阻塞）、`try_pop`、`close()`（关闭后 `pop` 取完剩余元素再返回空）
- `ThreadPool`：`submit(f, args...)` 返回 `std::future`；固定线程数；优雅关闭；任务抛出的异常通过 future 传回
- 任务类型使用你自己的 `MoveOnlyFunction`
- 选做：工作窃取（每线程一个队列）

**核查点**

- ☐ TSan 干净；压力测试：多生产者 / 多消费者，校验元素总和与个数
- ☐ 用 predicate 版 `wait`（或 `while` 循环）处理虚假唤醒，并说明原因
- ☐ 关闭时不死锁、不丢任务；析构语义写明（等待完成还是丢弃）并有测试
- ☐ 任务抛异常不会杀死工作线程；异常能从 `future.get()` 抛出
- ☐ 空闲时 CPU 占用接近 0（无忙等）
- ☐ 从任务里再 `submit` 不死锁（或明确写出限制）

**让 AI 分析什么**

- 「找出所有可能的死锁 / 数据竞争 / 丢唤醒场景」
- 「锁粒度与临界区是否可以缩小」
- 「关闭流程在竞态下是否会漏掉任务」

**优化与扩展**

- 批量出队减少锁争用；`notify_one` 与 `notify_all` 的取舍
- 与无锁队列（项目 9）对比

**通过标准**：TSan 与压力测试连续跑 100 次无失败。

## 项目 9：SPSC 无锁环形队列

**训练目标**：`std::atomic`、memory order（acquire / release）、false sharing、2 的幂容量 + 位掩码；这是「传感器 → 控制环」实时数据通道的基础。

**必须实现**

- `SpscQueue<T, N>`：N 必须是 2 的幂（`static_assert` 检查）；`try_push`、`try_pop`、`empty`、`size_approx`；一个生产者线程、一个消费者线程
- 元素用原始存储 + 就地构造 / 析构，支持非平凡类型
- 读、写下标放在不同缓存行（`alignas(64)`）；可缓存对端下标以减少跨核读取

**核查点**

- ☐ TSan 干净；百万元素压力测试，顺序与内容完全一致
- ☐ 为每个 atomic 操作写出所选 memory order 与理由；说明改成 relaxed 会在什么场景出错
- ☐ 满 / 空判定正确，下标回绕（含 `size_t` 回绕）正确
- ☐ false sharing 实验：有 / 无 padding 的吞吐对比
- ☐ 非平凡类型（含析构、`unique_ptr`）正确构造 / 析构、不泄漏；队列析构时清理剩余元素
- ☐ 与项目 8 的 mutex 版队列对比吞吐与延迟分布

**让 AI 分析什么**

- 「审查每个 atomic 的 memory order 是否足够 / 过强，画出 happens-before 链」
- 「找出回绕、对象生命周期方面的问题」
- 「为什么 SPSC 不需要 CAS」

**优化与扩展**

- MPSC 版本；日志写入通道；批量 push / pop
- 用 seqlock 实现「最新值」读写（传感器最新状态）

**通过标准**：核查点有证据；能在白板上讲清每个 atomic 的作用。

## 项目 10：迷你发布订阅与执行器（rclcpp 思路）

**训练目标**：回调生命周期、`shared_ptr` 消息传递、`weak_ptr` 观察者、线程模型、定时器、优雅停止。

**必须实现**

- `Topic<Msg>`、`Publisher<Msg>`、`Subscription<Msg>`：订阅返回 RAII 句柄，析构时自动退订
- 消息以 `shared_ptr<const Msg>` 传递（多个订阅者零拷贝）
- `Executor`：单线程与多线程两种；`spin_once` / `spin` / `stop`；回调队列
- `Timer`：周期回调，基于 `steady_clock`
- 订阅者可设队列深度，满时的丢弃策略（丢旧 / 丢新）
- 选做：回调组（互斥组 / 可重入组）

**核查点**

- ☐ 订阅句柄析构后不再收到回调，也不崩溃；回调执行期间被退订如何处理，写出设计并测试
- ☐ 回调里再 publish / subscribe 不死锁
- ☐ 消息生命周期：最后一个订阅者处理完才释放（析构计数验证）
- ☐ 慢回调不拖垮其他订阅者（队列深度与丢弃策略生效）
- ☐ 多线程 executor 下 TSan 干净；互斥回调组内的回调不并发
- ☐ 定时器漂移测试：连续 1000 个周期的误差统计

**让 AI 分析什么**

- 「回调捕获 this 的生命周期问题与悬空风险」
- 「找出死锁场景」
- 「我的设计与 rclcpp 有哪些差异，各自的原因」

**优化与扩展（ROS2 衔接）**

- 对照阅读 rclcpp 的 `Node`、`create_subscription`、`SingleThreadedExecutor` / `MultiThreadedExecutor`、`CallbackGroup`、intra-process 通信
- 研究 lambda 捕获 `this` / `shared_from_this` 在回调里的生命周期坑

**通过标准**：核查点有证据；能口述一条消息从 publish 到回调执行的完整路径。

## 项目 11：零分配实时控制环

**训练目标**：把项目 6、9 组合起来：运行期零堆分配、零锁，并测量周期抖动。

**必须实现**

- 一个 1 kHz 控制循环线程：从 SPSC 队列取传感器数据，做计算（例如 PD 控制），把指令写入另一个 SPSC 队列
- 一个模拟传感器线程（生产数据）和一个模拟执行器线程（消费指令）
- 启动阶段预分配所有内存（Arena / 池），运行阶段不 new / malloc
- 周期由 `steady_clock` + `sleep_until` 驱动，记录每个周期的实际耗时与抖动

**核查点**

- ☐ 零分配：重载全局 `operator new` 计数，运行阶段为 0
- ☐ 输出周期误差的 p50 / p99 / 最大值
- ☐ 对比实验：故意在循环里加一次 `new` 或 mutex，观察最坏抖动的变化
- ☐ 控制路径里无异常抛出、无意外的动态内存（用 `noexcept` 标注并检查）
- ☐ 在 Linux 上实验：线程优先级（SCHED\_FIFO）、CPU 亲和性、`mlockall`、启动期预触页；记录前后抖动差异

**让 AI 分析什么**

- 「帮我找出循环里隐藏的分配：容器扩容、`std::string`、`std::function`、iostream」
- 「分析抖动长尾可能来自哪里」

**优化与扩展**

- 加入看门狗（超时检测）与安全停机
- 把抖动数据画成直方图，对比不同调度配置

**通过标准**：核查点有证据；能解释每一处「为什么这里不能分配内存 / 加锁」。

## 项目 12：小型数学库（Vec / Mat / Quat / Transform）

**训练目标**：数值模板、constexpr、运算符重载、内存布局、数值稳定性；机器人常用的位姿表示。

**必须实现**

- `Vec<T, N>`、`Mat<T, R, C>`、`Quat<T>`、`Transform<T>`（旋转 + 平移）；用 `std::floating_point` 约束 T
- Vec：加减、标量乘、点积、叉积（3 维）、范数、归一化
- Mat：乘法、转置、行列式、逆（3×3、4×4）
- Quat：乘法、共轭、归一化、`slerp`；与旋转矩阵 / 轴角 / 欧拉角互转
- Transform：组合、求逆、变换点与向量
- 全部可 `constexpr`；类型为标准布局且可平凡拷贝（便于直接 memcpy 进消息或共享内存）

**核查点**

- ☐ 用 `static_assert` 做编译期测试（constexpr 构造与运算）
- ☐ 与已知值（或 Eigen）对比，double 误差小于 1e-9
- ☐ 边界：零向量归一化、接近 180° 的 `slerp`、万向锁处的欧拉角转换
- ☐ `is_standard_layout_v` 与 `is_trivially_copyable_v` 断言通过
- ☐ 实验：单位四元数连乘 1 万次后是否需要重新归一化，观察漂移
- ☐ benchmark：同尺寸矩阵乘法，对比 Eigen

**让 AI 分析什么**

- 「数值稳定性审查：浮点比较、消去误差」
- 「运算符重载是否产生多余临时对象」
- 「成员布局与对齐是否合理」

**优化与扩展**

- 表达式模板避免临时对象
- 用本库实现平面 2 连杆、空间 3 自由度机械臂的正运动学与雅可比矩阵，并用数值微分验证
- SIMD 友好布局实验（SoA 与 AoS）

**通过标准**：核查点有证据；运动学例子与数值微分结果一致。

## 项目 13：ROS2 综合落地（收官项目）

**训练目标**：把前面的内功用在真实框架里，并能对照理解 rclcpp。

**必须实现**

- 选当前稳定的 LTS 发行版，写一个 C++ 包：订阅传感器话题、发布控制指令话题、带一个周期定时器
- 用项目 12 的数学库做位姿计算；用项目 11 的方法避免控制回调里的动态分配
- 把节点写成组合节点（component），并尝试 intra-process 通信
- 用 `ros2 topic hz` / `ros2 topic delay` 验证频率与延迟

**核查点**

- ☐ 回调里无动态分配（用项目 11 的 `operator new` 计数方法验证）
- ☐ 能说出回调组配置的理由
- ☐ 开启 intra-process 后消息拷贝次数下降（用带 Tracer 的消息类型验证）
- ☐ 能对照项目 10：说出 rclcpp 的每个部件对应你写的哪个部件

**让 AI 分析什么**

- 「review 回调线程模型、消息所有权、QoS 配置是否合理」

**通过标准**：节点稳定运行，核查点有证据。

## 附录 A：与 AI 协作的提问模板

**1. 代码审查（写完一个项目后）**

```
我完成了【项目名】，附代码和测试。请按下面维度逐条审查，输出「问题 — 原因 — 修改思路」，不要直接给完整代码：
1. UB 与对象生命周期
2. 异常安全等级
3. const / noexcept / 值类别
4. 复杂度与多余拷贝
5. API 设计
6. 对应的 Core Guidelines 条款
7. 缺失的测试
```

**2. 卡住时**

```
现象：
最小复现：
我的假设：
我已经排查过：
我想搞懂的概念：
```

**3. 对抗测试**：`请为我的【组件】设计 10 个最可能暴露 bug 的测试用例，只给用例描述和预期结果，不给实现。`

**4. 概念追问**：`请用我这个项目里的代码举例，解释【概念】，并指出我的代码哪里用对了、哪里没用对。`

**5. 读源码**：`我贴出 libstdc++ 的这一段，请逐段解释设计意图，并告诉我我的实现缺了哪些考虑。`

**6. 面试官模式**：`请扮演面试官，围绕我的【项目】连续追问 5 个问题；我回答后你再点评，不要先给标准答案。`

**7. 默写检查**：`这是我隔几天后从零默写的核心代码，请对比我之前的版本，指出我还没真正理解的地方。`

## 附录 B：优化方法清单

顺序：正确性，其次安全，最后性能。没有测试覆盖的代码不优化。

- **先测量**：Google Benchmark（预热、多次迭代、`DoNotOptimize`）；用 `perf`、火焰图、`valgrind --tool=callgrind` 找热点
- **一次只改一件事**，记录前后数据
- **常见方向**：减少分配（`reserve`、池、SBO）；减少拷贝（移动、`emplace`、`string_view` / `span`）；缓存友好（连续存储、SoA、少追指针）；减少分支与锁争用
- **编译器协助**：`-O2` / `-O3`、LTO、PGO；用 Compiler Explorer 看汇编，确认优化真的发生
- **安全网**：优化后重新跑全部 sanitizer 与差分测试
- **记录**：每次优化在 notes 里写一行「假设 — 改动 — 数据 — 结论」

## 附录 C：持续补充与复盘机制

### 错题本模板（notes/mistakes.md，一个问题一条）

- 现象
- 根因
- 正确做法
- 对应的 Core Guidelines 或标准条款
- 复现代码路径

### 每周复盘（15 分钟）

- 本周完成了什么
- 最大的一个坑是什么
- 下周要从零重写哪一段
- 要问 AI 的三个问题

### 能力自评（每阶段末打分，0–3）

评分含义：0 没接触；1 能照着写；2 能独立写并发现问题；3 能讲清取舍并指导别人。

- 值语义与传参
- RAII 与所有权
- 移动语义
- 模板与 concepts
- 异常安全
- 并发与内存模型
- 性能分析
- 调试与工具链
- 框架理解（ROS2）

### 持续补充规则

- 每遇到一个不懂的概念，在 notes/labs/ 里写一个 50 行以内的小实验，跑通并写 3 行结论
- 每个阶段结束，用新学到的东西重构旧项目一次
- 每月挑一个标准库组件（如 `optional`、`variant`、`span`）手写简化版
- 全部项目完成后，选 1–2 个开源机器人项目的 C++ 代码，用本手册的核查点做 review 练习

## 附录 D：阅读与资源清单

- **C++ Core Guidelines**：优先读 R（资源）、C（类）、E（错误）、T（模板）、CP（并发）
- **书**：《A Tour of C++》（快速全览）；《Effective Modern C++》（移动语义、智能指针、完美转发）；《C++ Concurrency in Action》（并发与内存模型）；《C++ Templates: The Complete Guide》（模板进阶，按需读）
- **在线**：cppreference、Compiler Explorer、CppCon 的 Back to Basics 系列
- **源码**：libstdc++ / libc++ 里 vector、string、shared\_ptr、function 的实现（对照，不要抄）
- **ROS2**：官方文档、rclcpp 源码、ros2\_control 的实时设计文档

## 附录 E：总验收清单

| 阶段 | 项目 | 完成 | 通过 AI review | 已写复盘 |
| --- | --- | --- | --- | --- |
| P1 | 项目 1 学生管理系统 v1 / v2 / v3 | ☐ | ☐ | ☐ |
| P2 | 项目 2 UniquePtr / SharedPtr / WeakPtr | ☐ | ☐ | ☐ |
| P2 | 项目 3 String | ☐ | ☐ | ☐ |
| P3 | 项目 4 List | ☐ | ☐ | ☐ |
| P3 | 项目 5 Vector | ☐ | ☐ | ☐ |
| P3 | 项目 6 Arena 与对象池 | ☐ | ☐ | ☐ |
| P4 | 项目 7 Function | ☐ | ☐ | ☐ |
| P4 | 项目 8 线程安全队列与线程池 | ☐ | ☐ | ☐ |
| P4 | 项目 9 SPSC 无锁队列 | ☐ | ☐ | ☐ |
| P5 | 项目 10 迷你发布订阅与执行器 | ☐ | ☐ | ☐ |
| P5 | 项目 11 零分配实时控制环 | ☐ | ☐ | ☐ |
| P5 | 项目 12 小型数学库 | ☐ | ☐ | ☐ |
| P5 | 项目 13 ROS2 综合落地 | ☐ | ☐ | ☐ |

### 毕业标准（全部做到才算内功达标）

- 不看资料手写 UniquePtr、Vector 核心、SPSC 队列，并讲清每个决策
- 拿到一段陌生 C++ 代码，能指出其中的生命周期、异常安全、并发隐患
- 能解释 rclcpp 里回调、执行器、消息所有权的设计，并说出自己实现与它的差异
- 错题本中同类错误连续两个项目不再出现

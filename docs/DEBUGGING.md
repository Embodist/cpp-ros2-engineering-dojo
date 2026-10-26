# Debug / Sanitizers / Benchmark 最小工作流

## 构建与警告
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build -R dojo_03_ --output-on-failure
```

## ASan + UBSan（生命周期/越界/UB）
```bash
python3 tools/train.py test 03 --sanitize asan
# 等效 cmake -S . -B build-asan -DDOJO_SANITIZE_ADDRESS=ON -DCMAKE_BUILD_TYPE=Debug
```

## TSan（竞态）
```bash
python3 tools/train.py test 08 --sanitize tsan
python3 tools/train.py test 09 --sanitize tsan
```
TSan 和 ASan 不可同时启用。TSan 在某些容器/WSL/内核映射上可能遇到工具自身映射失败；记录环境并在原生 Linux 或兼容环境重测。

## gdb
```bash
gdb --args build/stage1/dojo_test_03_mini_vector
(gdb) run
(gdb) bt
(gdb) frame 0
(gdb) print this->size_
```

## Valgrind（可选）
`valgrind --leak-check=full ./build/stage1/dojo_test_03_mini_vector`，在支持的平台上使用；与 ASan 独立测试。

## benchmark
先确定工作负载，再在 Release 构建，预热、多次运行、统计 p50/p99；不能用编译器优化消除掉的空循环作性能结论。记录硬件、OS、编译器、数据规模、重复次数和原始输出。

# STL vector 源码：带着 03 MiniVector 进去

1. 记录编译器 `g++ --version`，确认用 libstdc++ 还是 libc++，记录头文件路径；不要把不同实现混写成“C++ 标准规定”。
2. 搜索 `vector` 扩容相关函数（函数名与版本有关），画出 allocate → construct → move/copy → destroy → deallocate 的顺序。
3. 用自定义 `Tracked` 类型统计构造、拷贝、移动、析构；再换成复制抛异常、移动非 noexcept 的类型。
4. 记录为什么“申请存储”和“构造对象”不同；解释何时不能强异常保证、哪些迭代器失效。
5. 设计 2 个能让自己实现失败、成熟容器成功的测试，给出原因；**不复制标准库代码到练习**。

交付 `notes/source-stl-vector.md`：版本、调用链、生命周期图、测试及日志。

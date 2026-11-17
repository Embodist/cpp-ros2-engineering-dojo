#pragma once
// 自定义异常层次（项目 1 v3 要求）
// ============================================================================
// 为什么不用裸 std::runtime_error：
//   调用方需要能区分"数据非法"和"IO 失败"，才能决定重试还是报错给用户。
//   用继承 std::runtime_error 的自定义类型，catch(const ValidationError&) 和
//   catch(const IoError&) 就能精确分流，同时仍能被 catch(const std::exception&) 兜底。
//
// 参考: Core Guidelines E.14（用专门类型而非通用类型表示错误）；
//       E.15（抛对象、按引用 catch，别按值 catch 造成切片）。
// ============================================================================

#include <stdexcept>
#include <string>

// 输入数据非法（重复 id、空姓名、成绩越界……）
class ValidationError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;  // 继承构造函数
};

// 文件读写失败或内容损坏
class IoError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

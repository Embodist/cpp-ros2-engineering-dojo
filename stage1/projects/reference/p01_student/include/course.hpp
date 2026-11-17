#pragma once
// Course —— 课程，与学生多对多（项目 1 v2 阶段 b）
// ============================================================================
// 核心训练点：**所有权**。学生由 Repository 用 shared_ptr 拥有；课程只是"引用"学生，
// 若也用 shared_ptr 互相持有，就会循环引用 → 双方都永不析构 → 泄漏。
// 正解：课程持有 weak_ptr<Student>（观察，不拥有），需要时 lock() 提升。
// ============================================================================

#include <memory>
#include <string>
#include <vector>

class Student;

class Course {
public:
    Course(int id, std::string name);

    int id() const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }

    // 选课：只存弱引用。易错点: 传 shared_ptr 参数是为了调用方方便；
    //       内部转成 weak_ptr 存起来，不增加强引用计数。
    void enroll(const std::shared_ptr<Student>& student);

    // 退课（按学号）。返回是否确实删掉了一个。
    bool drop(int student_id);

    // 当前有效学生（lock 成功的）。弱引用对端已析构的会被自动跳过。
    std::vector<std::shared_ptr<Student>> students() const;

    // 有效学生数（不含已析构的）
    std::size_t active_count() const;

private:
    int id_;
    std::string name_;
    // 易错点: 这里是 weak_ptr 不是 shared_ptr，正是为了打破循环引用。
    std::vector<std::weak_ptr<Student>> students_;
};

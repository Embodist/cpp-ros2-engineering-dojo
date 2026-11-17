#include "course.hpp"
#include "person.hpp"

Course::Course(int id, std::string name) : id_(id), name_(std::move(name)) {}

void Course::enroll(const std::shared_ptr<Student>& student) {
    if (!student) return;
    for (const auto& w : students_) {
        // 已存在的（且仍存活）就不要重复加入
        if (auto s = w.lock(); s && s->id() == student->id()) return;
    }
    students_.emplace_back(student);  // weak_ptr 从 shared_ptr 构造，count 不变
}

bool Course::drop(int student_id) {
    for (auto it = students_.begin(); it != students_.end(); ++it) {
        if (auto s = it->lock(); s && s->id() == student_id) {
            students_.erase(it);
            return true;
        }
    }
    // 也清理一下已经过期的（对端已析构）记录
    std::erase_if(students_, [](const std::weak_ptr<Student>& w) { return w.expired(); });
    return false;
}

std::vector<std::shared_ptr<Student>> Course::students() const {
    std::vector<std::shared_ptr<Student>> out;
    for (const auto& w : students_) {
        // 为什么用 lock：weak_ptr 不能直接访问对象；lock 在有强引用时返回有效 shared_ptr，
        // 否则返回空。这正是"删除学生后课程不再指向已销毁对象"的实现方式。
        if (auto s = w.lock()) out.push_back(std::move(s));
    }
    return out;
}

std::size_t Course::active_count() const {
    std::size_t n = 0;
    for (const auto& w : students_)
        if (!w.expired()) ++n;
    return n;
}

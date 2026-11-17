#pragma once
// Repository —— 人员仓库（项目 1 v1 的增删改查 + v2 多态存储 + v3 强异常保证）
// ============================================================================
// 所有权：仓库用 shared_ptr<Person> 拥有所有 Person；对外查询返回【观察指针】(Person*)，
// 不转移所有权。因为对象在堆上、由 shared_ptr 管理，返回的指针在对象存活期内稳定——
// 这正好解决 v1 里 "vector<Student> 扩容导致指针失效" 的问题（见 find_by_id 注释）。
// ============================================================================

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Person;
class Student;

class Repository {
public:
    Repository() = default;

    // ---------- 增删改 ----------
    // 重复 id 抛 ValidationError（不覆盖、不静默忽略）
    void add(std::shared_ptr<Person> person);

    bool remove(int id);   // 删到返回 true

    // 更新姓名/年龄（按 id）。找不到返回 false。
    bool update(int id, std::string name, int age);

    // ---------- 查 ----------
    // 返回观察指针；找不到返回 nullptr（空，而不是悬空）。
    Person* find_by_id(int id) const noexcept;
    Person* find_by_name(std::string_view name) const noexcept;

    // 供 Course 选课用：把某学生提升成 shared_ptr<Student>；不是学生或找不到返回空。
    std::shared_ptr<Student> share_student(int id) const;

    // 多态拷贝：克隆一个 Person（unique_ptr<Person>）。找不到返回空。
    std::unique_ptr<Person> clone_person(int id) const;

    // ---------- 排序 / 统计 ----------
    enum class Field { Id, Name, Age };
    // 按任意字段排序（std::ranges::sort + 投影）
    void sort_by(Field field);

    // 某科平均分；没有任何学生有该科成绩时返回 nullopt
    std::optional<double> average(std::string_view subject) const;

    // 某科前 n 名（按分数降序），返回观察指针
    std::vector<const Student*> top_n(std::string_view subject, std::size_t n) const;

    // ---------- 持久化 ----------
    void save(const std::string& path) const;   // 临时文件 + 原子重命名
    void load(const std::string& path);          // 损坏则抛 IoError，且仓库不变
    void import_batch(std::string_view text);    // 强异常保证：全成功或原样不变

    // ---------- 状态 ----------
    std::size_t size() const noexcept { return people_.size(); }
    const std::vector<std::shared_ptr<Person>>& people() const noexcept { return people_; }
    void clear() noexcept { people_.clear(); }

private:
    bool has_id(int id) const noexcept;

    std::vector<std::shared_ptr<Person>> people_;
};

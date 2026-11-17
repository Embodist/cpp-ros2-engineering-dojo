#pragma once
// 人员类型：抽象基类 Person + 派生 Student / Teacher（项目 1 v1+v2 合并形态）
// ============================================================================
// v1 的 Student 是个"值类型"（id/name/age/scores），v2 把它放进多态层次：
//   Person（抽象）── Student（有成绩表）／Teacher（有任教科目）
// 仓库用 vector<shared_ptr<Person>> 存，从而"学生/教师混存"且支持多态。
//
// 多态 + 值语义的关键张力：多态对象不能按值拷贝（会【对象切片】），
// 所以用虚函数 clone() 做多态拷贝。
// ============================================================================

#include <iosfwd>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

class Person {
public:
    // 实例计数：用来证明"排序/查询不会新建 Person"、"清空后构造数==析构数"
    inline static long constructed = 0;
    inline static long destroyed   = 0;
    static void reset_stats() noexcept { constructed = destroyed = 0; }

    Person(int id, std::string name, int age);
    // 易错点: 基类析构必须 virtual。否则 delete 一个 Person* 指向 Student 时，
    //         只调用 ~Person，Student 的成员（scores_）不析构 → 泄漏/UB。
    //         参考: Core Guidelines C.35（基类析构要么 public+virtual，要么 protected 非 virtual）。
    virtual ~Person();

    int  id()   const noexcept { return id_; }
    const std::string& name() const noexcept { return name_; }
    int  age()  const noexcept { return age_; }

    void set_name(std::string name);   // 会校验（非空、无分隔符）
    void set_age(int age);             // 会校验（>=0）

    // 多态拷贝。易错点: 不能用拷贝构造——`Person p = *derived_ptr` 会被切片成纯 Person。
    //         必须虚函数返回 unique_ptr<Person>，在派生类里 new 自己。
    virtual std::unique_ptr<Person> clone() const = 0;

    virtual std::string role() const = 0;             // "student" / "teacher"
    virtual void write(std::ostream& os) const = 0;   // 序列化成一行（供 save）

protected:
    int id_;
    std::string name_;
    int age_;
};

// ---------------- Student ----------------
class Student : public Person {
public:
    Student(int id, std::string name, int age);

    std::unique_ptr<Person> clone() const override;
    std::string role() const override { return "student"; }
    void write(std::ostream& os) const override;

    // 成绩：科目 -> 分数（0..100）。越界抛 ValidationError。
    void set_score(std::string_view subject, double score);
    std::optional<double> score(std::string_view subject) const;

    const std::map<std::string, double>& scores() const noexcept { return scores_; }

private:
    std::map<std::string, double> scores_;
};

// ---------------- Teacher ----------------
class Teacher : public Person {
public:
    Teacher(int id, std::string name, int age, std::string subject);

    std::unique_ptr<Person> clone() const override;
    std::string role() const override { return "teacher"; }
    void write(std::ostream& os) const override;

    const std::string& subject() const noexcept { return subject_; }

private:
    std::string subject_;
};

// ---------------- 校验 / 解析工具 ----------------
// 校验通过则返回，否则抛 ValidationError
void validate_name(std::string_view name);
void validate_age(int age);
void validate_score(double score);

// 把一行文本解析成 Person（供 load / import_batch 用）；非法则抛 ValidationError。
// 格式:  S,<id>,<name>,<age>,<subj>:<score>,...
//        T,<id>,<name>,<age>,<subject>
std::unique_ptr<Person> parse_person(std::string_view line);

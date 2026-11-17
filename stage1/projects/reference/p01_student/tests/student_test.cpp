// 项目 1 参考测试：覆盖「功能要求清单」里 p01 的每条功能验收 + 内功核查点。
#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "common/leak_check.hpp"
#include "course.hpp"
#include "errors.hpp"
#include "person.hpp"
#include "repository.hpp"

namespace {

// 小工具：把仓库里所有人的 id 收集成 vector，方便断言顺序
std::vector<int> ids(const Repository& r) {
    std::vector<int> v;
    for (const auto& p : r.people()) v.push_back(p->id());
    return v;
}

std::string make_temp_path(const std::string& name) {
    return (std::filesystem::temp_directory_path() / name).string();
}

}  // namespace

// ---------- 增删改查 ----------
TEST(Repository, AddRemoveUpdateReflected) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));
    r.add(std::make_shared<Student>(2, "Bob", 21));
    EXPECT_EQ(r.size(), 2u);

    r.update(1, "Alice2", 22);
    EXPECT_EQ(r.find_by_id(1)->name(), "Alice2");
    EXPECT_EQ(r.find_by_id(1)->age(), 22);

    EXPECT_TRUE(r.remove(2));
    EXPECT_EQ(r.size(), 1u);
    EXPECT_EQ(r.find_by_id(2), nullptr);
    EXPECT_FALSE(r.remove(999));  // 删不存在的返回 false，不崩
}

TEST(Repository, RejectsInvalidInput) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));

    EXPECT_THROW(r.add(std::make_shared<Student>(1, "Dup", 20)), ValidationError);  // id 冲突
    EXPECT_THROW(r.add(std::make_shared<Student>(2, "", 20)), ValidationError);     // 空姓名
    // 成绩越界在 set_score / 构造解析时挡
    auto s = std::make_shared<Student>(3, "Cara", 19);
    EXPECT_THROW(s->set_score("math", 101.0), ValidationError);
    EXPECT_THROW(s->set_score("math", -1.0), ValidationError);
    EXPECT_NO_THROW(s->set_score("math", 100.0));
}

TEST(Repository, FindReturnsNullWhenMissing) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));
    EXPECT_NE(r.find_by_id(1), nullptr);
    // 易错点: 返回的是"空"而不是悬空——因为返回 nullptr，调用方显式判空即可，不会踩野指针。
    EXPECT_EQ(r.find_by_id(42), nullptr);
    EXPECT_EQ(r.find_by_name("Nobody"), nullptr);
}

// ---------- 排序 / 统计 ----------
TEST(Repository, SortByEachField) {
    Repository r;
    r.add(std::make_shared<Student>(3, "Cara", 19));
    r.add(std::make_shared<Student>(1, "Alice", 22));
    r.add(std::make_shared<Student>(2, "Bob", 20));

    r.sort_by(Repository::Field::Id);
    EXPECT_EQ(ids(r), (std::vector<int>{1, 2, 3}));
    r.sort_by(Repository::Field::Name);
    EXPECT_EQ(ids(r), (std::vector<int>{1, 2, 3}));  // Alice, Bob, Cara
    r.sort_by(Repository::Field::Age);
    EXPECT_EQ(ids(r), (std::vector<int>{3, 2, 1}));  // 19,20,22
}

TEST(Repository, AverageAndTopN) {
    Repository r;
    auto a = std::make_shared<Student>(1, "Alice", 20);
    a->set_score("math", 90);
    auto b = std::make_shared<Student>(2, "Bob", 20);
    b->set_score("math", 80);
    auto c = std::make_shared<Student>(3, "Cara", 20);
    c->set_score("math", 100);
    r.add(a); r.add(b); r.add(c);

    auto avg = r.average("math");
    ASSERT_TRUE(avg.has_value());
    EXPECT_DOUBLE_EQ(*avg, 90.0);

    auto top = r.top_n("math", 2);
    ASSERT_EQ(top.size(), 2u);
    EXPECT_EQ(top[0]->id(), 3);   // 100
    EXPECT_EQ(top[1]->id(), 1);   // 90

    EXPECT_FALSE(r.average("history").has_value());  // 无此科 → 空
    EXPECT_TRUE(r.top_n("history", 3).empty());
}

// ---------- 持久化 ----------
TEST(Repository, SaveThenLoadRoundTrips) {
    const std::string path = make_temp_path("p01_roundtrip.csv");
    std::filesystem::remove(path);

    {
        Repository r;
        r.add(std::make_shared<Student>(1, "Alice", 20));
        auto s = std::make_shared<Student>(2, "Bob", 21);
        s->set_score("math", 88.5);
        r.add(s);
        r.add(std::make_shared<Teacher>(3, "Prof", 45, "physics"));
        r.save(path);
    }

    Repository loaded;
    loaded.load(path);
    ASSERT_EQ(loaded.size(), 3u);
    EXPECT_EQ(loaded.find_by_id(1)->name(), "Alice");
    EXPECT_EQ(loaded.find_by_id(3)->role(), "teacher");

    auto* bob = dynamic_cast<Student*>(loaded.find_by_id(2));
    ASSERT_NE(bob, nullptr);
    ASSERT_TRUE(bob->score("math").has_value());
    EXPECT_DOUBLE_EQ(*bob->score("math"), 88.5);

    std::filesystem::remove(path);
}

TEST(Repository, LoadCorruptFileLeavesRepoUnchanged) {
    const std::string path = make_temp_path("p01_corrupt.csv");
    {
        std::ofstream out(path);
        out << "S,1,Alice,20,math:90\n";
        out << "S,2,Bob,not_a_number\n";   // 损坏：年龄非法
    }

    Repository r;
    r.add(std::make_shared<Student>(9, "Existing", 30));

    // 训练点: 加载损坏文件抛 IoError，且仓库保持原样（不留半成品）。
    EXPECT_THROW(r.load(path), IoError);
    EXPECT_EQ(r.size(), 1u);
    EXPECT_EQ(r.find_by_id(9)->name(), "Existing");
    EXPECT_EQ(r.find_by_id(1), nullptr);   // 文件里的 Alice 没有被读进来

    std::filesystem::remove(path);
}

TEST(Repository, ImportBatchRollsBackOnMidFailure) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));

    // 第 2 行成绩越界 → 整批回滚
    const std::string batch =
        "S,2,Bob,21,math:70\n"
        "S,3,Cara,19,math:200\n";   // 非法
    EXPECT_THROW(r.import_batch(batch), ValidationError);

    // 状态与导入前逐字段相等
    EXPECT_EQ(r.size(), 1u);
    EXPECT_EQ(r.find_by_id(2), nullptr);
    EXPECT_EQ(r.find_by_id(3), nullptr);

    // 合法批次则成功并入
    r.import_batch("S,2,Bob,21,math:70\nS,3,Cara,19,math:95\n");
    EXPECT_EQ(r.size(), 3u);
}

// ---------- 多态 / 所有权 ----------
TEST(Person, PolymorphicClone) {
    Repository r;
    auto s = std::make_shared<Student>(1, "Alice", 20);
    s->set_score("math", 90);
    r.add(s);
    r.add(std::make_shared<Teacher>(2, "Prof", 45, "physics"));

    auto cloned = r.clone_person(1);
    ASSERT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->role(), "student");            // 动态类型保持为 Student（没被切片）
    auto* cs = dynamic_cast<Student*>(cloned.get());
    ASSERT_NE(cs, nullptr);
    EXPECT_EQ(cs->scores().at("math"), 90.0);

    // 克隆是独立对象：改克隆不影响原对象
    cs->set_score("math", 10);
    EXPECT_EQ(dynamic_cast<Student*>(r.find_by_id(1))->scores().at("math"), 90.0);
}

TEST(Person, MixedStorage) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));
    r.add(std::make_shared<Teacher>(2, "Prof", 45, "physics"));
    EXPECT_EQ(r.find_by_id(1)->role(), "student");
    EXPECT_EQ(r.find_by_id(2)->role(), "teacher");
}

TEST(Course, WeakReferenceBreaksCycle) {
    Repository r;
    r.add(std::make_shared<Student>(1, "Alice", 20));

    Course c(100, "Robotics");
    c.enroll(r.share_student(1));       // 只存弱引用
    EXPECT_EQ(c.active_count(), 1u);

    // 训练点: 学生被删（最后一个 shared_ptr 消失）后，课程里的 weak_ptr 过期，
    //         lock() 返回空，绝不访问已销毁对象。
    EXPECT_TRUE(r.remove(1));
    EXPECT_EQ(c.active_count(), 0u);
    EXPECT_TRUE(c.students().empty());
}

// ---------- 内功：无多余对象拷贝 / 资源平衡 ----------
TEST(Repository, SortDoesNotCopyPersons) {
    Repository r;
    for (int i = 0; i < 50; ++i)
        r.add(std::make_shared<Student>(i, "n" + std::to_string(i), 20));

    Person::reset_stats();
    r.sort_by(Repository::Field::Id);
    // 排序的是 vector<shared_ptr>，只交换指针，不构造任何 Person。
    EXPECT_EQ(Person::constructed, 0);
}

TEST(Repository, ConstructedEqualsDestroyed) {
    Person::reset_stats();
    {
        Repository r;
        r.add(std::make_shared<Student>(1, "Alice", 20));
        auto s = std::make_shared<Student>(2, "Bob", 21);
        s->set_score("math", 90);
        r.add(s);
        EXPECT_EQ(Person::constructed, 2);
    }
    // 仓库析构后，两个 Person 都该被销毁（无泄漏）
    EXPECT_EQ(Person::constructed, Person::destroyed);
}

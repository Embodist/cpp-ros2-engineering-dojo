#include "person.hpp"
#include "errors.hpp"

#include <cctype>
#include <charconv>
#include <ostream>
#include <sstream>
#include <vector>

// ============================ 校验 ============================
// 为什么集中在这里：校验规则只写一处，构造/修改/解析都复用，避免散落导致不一致。
void validate_name(std::string_view name) {
    if (name.empty())
        throw ValidationError("姓名不能为空");
    // 简化：不允许分隔符，以免破坏 CSV 格式（真实项目应做转义，见文件头注释）
    for (char c : name)
        if (c == ',' || c == '\n' || c == ':')
            throw ValidationError(std::string("姓名含非法字符: ") + std::string(name));
}

void validate_age(int age) {
    if (age < 0 || age > 150)
        throw ValidationError("年龄越界: " + std::to_string(age));
}

void validate_score(double score) {
    if (!(score >= 0.0 && score <= 100.0))  // 写成 !(a&&b) 可顺带挡住 NaN
        throw ValidationError("成绩越界(0..100)");
}

// ============================ Person ============================
Person::Person(int id, std::string name, int age) : id_(id), name_(std::move(name)), age_(age) {
    // 易错点: 先构造再校验是不行的——校验失败抛异常时对象没建成，但成员已构造，
    //         好在成员是 RAII，会正常析构，不会泄漏。这里在构造函数体内校验，
    //         用初始化列表把成员建好再验；若失败异常传出，成员自动析构。
    validate_name(name_);
    validate_age(age_);
    ++constructed;
}

Person::~Person() { ++destroyed; }

void Person::set_name(std::string name) {
    validate_name(name);
    name_ = std::move(name);
}

void Person::set_age(int age) {
    validate_age(age);
    age_ = age;
}

// ============================ Student ============================
Student::Student(int id, std::string name, int age) : Person(id, std::move(name), age) {}

std::unique_ptr<Person> Student::clone() const {
    // 用拷贝构造复制自己（含 scores_），再以基类指针返回——这就是"多态拷贝"
    return std::make_unique<Student>(*this);
}

void Student::set_score(std::string_view subject, double score) {
    if (subject.empty())
        throw ValidationError("科目不能为空");
    validate_score(score);
    scores_[std::string(subject)] = score;
}

std::optional<double> Student::score(std::string_view subject) const {
    auto it = scores_.find(std::string(subject));
    if (it == scores_.end()) return std::nullopt;  // 没这科 → 空，而不是 0 或崩溃
    return it->second;
}

void Student::write(std::ostream& os) const {
    os << "S," << id_ << ',' << name_ << ',' << age_;
    for (const auto& [subject, sc] : scores_)
        os << ',' << subject << ':' << sc;
}

// ============================ Teacher ============================
Teacher::Teacher(int id, std::string name, int age, std::string subject)
    : Person(id, std::move(name), age), subject_(std::move(subject)) {
    if (subject_.empty())
        throw ValidationError("任教科目不能为空");
}

std::unique_ptr<Person> Teacher::clone() const {
    return std::make_unique<Teacher>(*this);
}

void Teacher::write(std::ostream& os) const {
    os << "T," << id_ << ',' << name_ << ',' << age_ << ',' << subject_;
}

// ============================ 解析 ============================
namespace {

std::vector<std::string_view> split(std::string_view s, char delim) {
    std::vector<std::string_view> out;
    std::size_t start = 0;
    while (true) {
        std::size_t pos = s.find(delim, start);
        if (pos == std::string_view::npos) {
            out.push_back(s.substr(start));
            break;
        }
        out.push_back(s.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

// 严格把字符串转 int：不是纯数字就抛（不用 stoi，它接受 "12abc" 这种前缀）
int to_int(std::string_view sv) {
    int value = 0;
    const char* begin = sv.data();
    const char* end = sv.data() + sv.size();
    auto [ptr, ec] = std::from_chars(begin, end, value);
    if (ec != std::errc{} || ptr != end)
        throw ValidationError("非法整数: " + std::string(sv));
    return value;
}

double to_double(std::string_view sv) {
    // 注意: from_chars 对浮点支持在 GCC 11+ 才完整；这里用 std::stod 但要求整段消费
    std::string tmp(sv);
    std::size_t used = 0;
    double value = 0.0;
    try {
        value = std::stod(tmp, &used);
    } catch (const std::exception&) {
        throw ValidationError("非法浮点: " + tmp);
    }
    if (used != tmp.size())
        throw ValidationError("非法浮点: " + tmp);
    return value;
}

}  // namespace

std::unique_ptr<Person> parse_person(std::string_view line) {
    auto fields = split(line, ',');
    if (fields.size() < 4)
        throw ValidationError("字段过少: " + std::string(line));

    const std::string_view tag = fields[0];
    const int id  = to_int(fields[1]);
    const std::string name(fields[2]);
    const int age = to_int(fields[3]);
    validate_name(name);   // 解析入口也校验，保证任何来源的数据都过同一道关
    validate_age(age);

    if (tag == "S") {
        auto s = std::make_unique<Student>(id, name, age);
        for (std::size_t i = 4; i < fields.size(); ++i) {
            auto kv = split(fields[i], ':');
            if (kv.size() != 2)
                throw ValidationError("成绩字段格式应为 科目:分数 —— " + std::string(fields[i]));
            if (kv[0].empty())
                throw ValidationError("科目不能为空");
            double score = to_double(kv[1]);
            validate_score(score);
            s->set_score(kv[0], score);
        }
        return s;
    }
    if (tag == "T") {
        if (fields.size() != 5)
            throw ValidationError("教师记录应恰好 5 个字段: " + std::string(line));
        return std::make_unique<Teacher>(id, name, age, std::string(fields[4]));
    }
    throw ValidationError("未知角色标记: " + std::string(tag));
}

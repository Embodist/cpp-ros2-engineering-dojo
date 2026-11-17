#include "repository.hpp"
#include "errors.hpp"
#include "person.hpp"
#include "scope_guard.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ranges>
#include <system_error>
#include <utility>

// ============================ 增删改 ============================
void Repository::add(std::shared_ptr<Person> person) {
    if (!person) throw ValidationError("add: 空指针");
    if (has_id(person->id()))
        throw ValidationError("id 冲突: " + std::to_string(person->id()));
    people_.push_back(std::move(person));
}

bool Repository::remove(int id) {
    auto it = std::find_if(people_.begin(), people_.end(),
                           [id](const auto& p) { return p->id() == id; });
    if (it == people_.end()) return false;
    people_.erase(it);
    return true;
}

bool Repository::update(int id, std::string name, int age) {
    // 强异常保证的关键手法：**先全部校验，再统一施改**。
    // 若边校验边改，可能出现"名字改了、年龄校验失败抛异常"，对象处于半更新状态。
    validate_name(name);
    validate_age(age);

    Person* p = find_by_id(id);
    if (!p) return false;
    p->set_name(std::move(name));
    p->set_age(age);
    return true;
}

// ============================ 查 ============================
Person* Repository::find_by_id(int id) const noexcept {
    for (const auto& p : people_)
        if (p->id() == id) return p.get();
    return nullptr;
}

Person* Repository::find_by_name(std::string_view name) const noexcept {
    for (const auto& p : people_)
        if (p->name() == name) return p.get();
    return nullptr;
}

std::shared_ptr<Student> Repository::share_student(int id) const {
    for (const auto& p : people_)
        if (p->id() == id)
            // dynamic_pointer_cast：运行期确认确实是 Student，再提升为 shared_ptr<Student>。
            // 若不是（例如是 Teacher），返回空 —— 类型安全的"向下转换"。
            return std::dynamic_pointer_cast<Student>(p);
    return nullptr;
}

std::unique_ptr<Person> Repository::clone_person(int id) const {
    Person* p = find_by_id(id);
    return p ? p->clone() : nullptr;
}

// ============================ 排序 / 统计 ============================
void Repository::sort_by(Field field) {
    // 训练点: std::ranges::sort + 投影（第三个参数）。
    //   投影把"元素"映射成"比较键"，于是不用给每个字段写一个比较器。
    //   注意：排序的是 vector<shared_ptr>，只交换指针，**不拷贝 Person 对象**。
    switch (field) {
        case Field::Id:
            std::ranges::sort(people_, {}, [](const auto& p) { return p->id(); });
            break;
        case Field::Name:
            std::ranges::sort(people_, {}, [](const auto& p) { return p->name(); });
            break;
        case Field::Age:
            std::ranges::sort(people_, {}, [](const auto& p) { return p->age(); });
            break;
    }
}

std::optional<double> Repository::average(std::string_view subject) const {
    double sum = 0.0;
    std::size_t count = 0;
    for (const auto& p : people_) {
        if (auto* s = dynamic_cast<const Student*>(p.get())) {
            if (auto sc = s->score(subject)) {
                sum += *sc;
                ++count;
            }
        }
    }
    if (count == 0) return std::nullopt;   // 没有数据 → 空，而不是除零 / 0
    return sum / static_cast<double>(count);
}

std::vector<const Student*> Repository::top_n(std::string_view subject, std::size_t n) const {
    std::vector<std::pair<const Student*, double>> ranked;
    for (const auto& p : people_) {
        if (auto* s = dynamic_cast<const Student*>(p.get())) {
            if (auto sc = s->score(subject))
                ranked.emplace_back(s, *sc);
        }
    }
    // 降序：用 std::greater 作为比较器，投影取分数
    std::ranges::sort(ranked, std::greater<>{}, [](const auto& pr) { return pr.second; });

    std::vector<const Student*> out;
    for (std::size_t i = 0; i < ranked.size() && i < n; ++i)
        out.push_back(ranked[i].first);
    return out;
}

// ============================ 持久化 ============================
void Repository::save(const std::string& path) const {
    const std::string tmp = path + ".tmp";
    // 训练点: "先写临时文件、成功后原子重命名"。
    //   rename 在同一文件系统上是原子的：要么旧文件还在、要么新文件完整可见，
    //   绝不会出现"写了一半的半个文件"。ScopeGuard 保证失败时删掉临时文件。
    ScopeGuard cleanup([&tmp] {
        std::error_code ec;
        std::filesystem::remove(tmp, ec);  // 失败无所谓，尽力清理
    });

    std::ofstream out(tmp, std::ios::binary | std::ios::trunc);
    if (!out) throw IoError("无法创建临时文件: " + tmp);

    for (const auto& p : people_) {
        p->write(out);
        out << '\n';
    }
    out.flush();
    if (!out) throw IoError("写入失败: " + tmp);
    out.close();

    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) throw IoError("原子重命名失败: " + ec.message());

    cleanup.dismiss();  // 成功：撤销清理
}

void Repository::load(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw IoError("无法打开文件: " + path);

    // 训练点: 强异常保证 —— 先解析进【临时】容器，全部成功后才 swap 提交。
    //   任何一行损坏都在 swap 之前抛出，people_ 保持原样（不留半成品）。
    std::vector<std::shared_ptr<Person>> staged;
    std::string line;
    std::size_t lineno = 0;
    while (std::getline(in, line)) {
        ++lineno;
        if (!line.empty() && line.back() == '\r') line.pop_back();  // 兼容 CRLF
        if (line.empty()) continue;
        try {
            staged.push_back(parse_person(line));   // unique_ptr → shared_ptr 隐式转换
        } catch (const ValidationError& e) {
            throw IoError("第 " + std::to_string(lineno) + " 行损坏: " + e.what());
        }
    }
    if (in.bad()) throw IoError("读取过程中出错: " + path);

    people_.swap(staged);   // 提交点：到这里才改变仓库
}

void Repository::import_batch(std::string_view text) {
    // 训练点: copy-and-swap（强异常保证）。
    //   staging 是 people_ 的一份浅拷贝（只复制 shared_ptr，代价 O(n) 个指针）。
    //   全程只改 staging；任何一行非法就抛，people_ 丝毫未动。
    //   代价：每次导入 O(n) 拷贝指针；换来"要么全成功、要么完全不变"的强保证。
    std::vector<std::shared_ptr<Person>> staging = people_;

    std::size_t lineno = 0;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t nl = text.find('\n', start);
        std::string_view line = (nl == std::string_view::npos)
                                    ? text.substr(start)
                                    : text.substr(start, nl - start);
        start = (nl == std::string_view::npos) ? text.size() + 1 : nl + 1;
        ++lineno;
        if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
        if (line.empty()) continue;

        std::shared_ptr<Person> p = parse_person(line);  // 可能抛 ValidationError
        // id 冲突检查（对已有 + 本批已并入 staging 的）
        for (const auto& e : staging)
            if (e->id() == p->id())
                throw ValidationError("第 " + std::to_string(lineno) +
                                      " 行 id 冲突: " + std::to_string(p->id()));
        staging.push_back(std::move(p));
    }

    people_.swap(staging);   // 全部成功，一次性提交
}

// ============================ 内部 ============================
bool Repository::has_id(int id) const noexcept {
    for (const auto& p : people_)
        if (p->id() == id) return true;
    return false;
}

#pragma once
#include <exception>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace dojo::test {
using TestFunction = void (*)();
struct Case { std::string name; TestFunction fn; };
inline std::vector<Case>& cases() { static std::vector<Case> c; return c; }
struct Registrar { Registrar(const char* name, TestFunction fn) { cases().push_back({name, fn}); } };
struct AssertionFailure : std::runtime_error { using std::runtime_error::runtime_error; };
inline void require(bool ok, const char* expr, int line) {
    if (!ok) throw AssertionFailure(std::string("line ") + std::to_string(line) + ": " + expr);
}
inline int run() {
    int fails=0;
    for(const auto& c: cases()) {
        try { c.fn(); std::cout << "[PASS] " << c.name << '\n'; }
        catch(const std::exception& e) { ++fails; std::cerr << "[FAIL] " << c.name << ": " << e.what() << '\n'; }
        catch(...) { ++fails; std::cerr << "[FAIL] " << c.name << ": unknown exception\n"; }
    }
    std::cout << "Result: " << (cases().size()-static_cast<std::size_t>(fails)) << "/" << cases().size() << " tests passed\n";
    return fails == 0 ? 0 : 1;
}
}
#define DOJO_JOIN_IMPL(x,y) x##y
#define DOJO_JOIN(x,y) DOJO_JOIN_IMPL(x,y)
#define DOJO_TEST(name) \
    static void DOJO_JOIN(dojo_case_, __LINE__)(); \
    static ::dojo::test::Registrar DOJO_JOIN(dojo_reg_, __LINE__)(name, &DOJO_JOIN(dojo_case_, __LINE__)); \
    static void DOJO_JOIN(dojo_case_, __LINE__)()
#define REQUIRE(expr) ::dojo::test::require(static_cast<bool>(expr), #expr, __LINE__)
#define REQUIRE_EQ(a,b) REQUIRE((a) == (b))
#define REQUIRE_THROWS(expr) do { bool dojo_thrown=false; try { (void)(expr); } catch(const std::exception&) { dojo_thrown=true; } REQUIRE(dojo_thrown); } while(false)
#define DOJO_MAIN() int main() { return ::dojo::test::run(); }

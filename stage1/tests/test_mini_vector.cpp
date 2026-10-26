#include "dojo/mini_vector.hpp"
#include "test_harness.hpp"
#include <memory>
#include <string>
#include <utility>
#include <stdexcept>
DOJO_TEST("push, access, growth") { dojo::MiniVector<int> v; REQUIRE_EQ(v.size(),0u); for(int i=0;i<20;++i) v.push_back(i); REQUIRE_EQ(v.size(),20u); REQUIRE(v.capacity()>=20); REQUIRE_EQ(v.at(10),10); REQUIRE_THROWS(v.at(20)); }
DOJO_TEST("copy is deep") { dojo::MiniVector<std::string> v; v.push_back("hello"); auto w=v; w.at(0)="other"; REQUIRE_EQ(v.at(0),"hello"); REQUIRE_EQ(w.at(0),"other"); }
DOJO_TEST("move-only types") { dojo::MiniVector<std::unique_ptr<int>> v; v.push_back(std::make_unique<int>(4)); v.push_back(std::make_unique<int>(9)); REQUIRE_EQ(*v.at(1),9); }
DOJO_TEST("move container") { dojo::MiniVector<int> v; v.push_back(5); auto w=std::move(v); REQUIRE_EQ(w.at(0),5); REQUIRE_EQ(v.size(),0u); }


struct Tracked {
    static inline int alive = 0;
    static inline int copies_before_throw = -1;
    int value = 0;
    explicit Tracked(int v) : value(v) { ++alive; }
    Tracked(const Tracked& x) : value(x.value) {
        if (copies_before_throw == 0) throw std::runtime_error("copy fault");
        if (copies_before_throw > 0) --copies_before_throw;
        ++alive;
    }
    Tracked(Tracked&& x) noexcept(false) : value(x.value) { ++alive; }
    ~Tracked() { --alive; }
};
DOJO_TEST("reserve failure retains previous elements and frees temporaries") {
    REQUIRE_EQ(Tracked::alive, 0);
    {
        dojo::MiniVector<Tracked> v;
        Tracked a(1), b(2);
        v.push_back(a);
        v.push_back(b);
        Tracked::copies_before_throw = 0;
        REQUIRE_THROWS(v.reserve(v.capacity() + 10));
        Tracked::copies_before_throw = -1;
        REQUIRE_EQ(v.size(), 2u);
        REQUIRE_EQ(v.at(0).value, 1);
        REQUIRE_EQ(v.at(1).value, 2);
    }
    REQUIRE_EQ(Tracked::alive, 0);
}

DOJO_MAIN()

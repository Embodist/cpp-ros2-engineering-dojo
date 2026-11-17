#include "dojo/scope_exit.hpp"
#include "test_harness.hpp"
#include <stdexcept>
#include <utility>
DOJO_TEST("scope executes once") { int n=0; { dojo::ScopeExit g([&] { ++n; }); } REQUIRE_EQ(n,1); }
DOJO_TEST("release disables callback") { int n=0; { dojo::ScopeExit g([&] { ++n; }); g.release(); } REQUIRE_EQ(n,0); }
DOJO_TEST("move transfers ownership") { int n=0; { dojo::ScopeExit a([&] { ++n; }); dojo::ScopeExit b(std::move(a)); } REQUIRE_EQ(n,1); }
DOJO_TEST("exception unwinding") { int n=0; try { dojo::ScopeExit g([&]{++n;}); throw std::runtime_error("x"); } catch(const std::runtime_error&) {} REQUIRE_EQ(n,1); }
DOJO_TEST("move-assignment disposes previously owned callback") {
    int a=0,b=0;
    {
        dojo::ScopeExit old([&]{++a;});
        dojo::ScopeExit now([&]{++b;});
        now=std::move(old);
        REQUIRE_EQ(b,1);
    }
    REQUIRE_EQ(a,1);
    REQUIRE_EQ(b,1);
}
DOJO_TEST("self move keeps a single cleanup") {
    int n = 0;
    {
        dojo::ScopeExit g([&] { ++n; });
        dojo::ScopeExit& alias = g;
        g = std::move(alias);
    }
    REQUIRE_EQ(n, 1);
}
DOJO_TEST("move from released is inert") {
    int n = 0;
    {
        dojo::ScopeExit a([&] { ++n; });
        a.release();
        dojo::ScopeExit b(std::move(a));
        (void)b;
    }
    REQUIRE_EQ(n, 0);
}
DOJO_MAIN()

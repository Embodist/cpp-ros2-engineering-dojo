#include "dojo/unique_handle.hpp"
#include "test_harness.hpp"
#include <utility>
#include <vector>
DOJO_TEST("handle closes once") { std::vector<int> closed; { dojo::UniqueHandle h(12,[&](int x){closed.push_back(x);}); REQUIRE(h.valid()); } REQUIRE_EQ(closed.size(),1u); REQUIRE_EQ(closed[0],12); }
DOJO_TEST("release relinquishes ownership") { int closed=0; { dojo::UniqueHandle h(7,[&](int){++closed;}); REQUIRE_EQ(h.release(),7); REQUIRE(!h.valid()); } REQUIRE_EQ(closed,0); }
DOJO_TEST("reset and move") { std::vector<int> closed; { dojo::UniqueHandle a(1,[&](int x){closed.push_back(x);}); a.reset(2); dojo::UniqueHandle b(std::move(a)); REQUIRE(!a.valid()); REQUIRE_EQ(b.get(),2); } REQUIRE_EQ(closed.size(),2u); REQUIRE_EQ(closed[0],1); REQUIRE_EQ(closed[1],2); }
DOJO_MAIN()

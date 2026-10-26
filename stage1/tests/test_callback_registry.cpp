#include "dojo/callback_registry.hpp"
#include "test_harness.hpp"
#include <vector>
#include <stdexcept>
DOJO_TEST("ordered subscriptions") { dojo::CallbackRegistry r; std::vector<int> seen; auto t1=r.subscribe([&](int x){seen.push_back(x);}); r.subscribe([&](int x){seen.push_back(x+1);}); r.publish(9); REQUIRE_EQ(seen.size(),2u); REQUIRE_EQ(seen[0],9); REQUIRE_EQ(seen[1],10); REQUIRE(r.unsubscribe(t1)); REQUIRE(!r.unsubscribe(t1)); }
DOJO_TEST("self unsubscribe and snapshot") { dojo::CallbackRegistry r; int n=0; dojo::CallbackRegistry::Token t=0; t=r.subscribe([&](int){++n; r.unsubscribe(t); r.subscribe([&](int){n+=10;});}); r.publish(1); REQUIRE_EQ(n,1); r.publish(2); REQUIRE_EQ(n,11); }
DOJO_TEST("reject empty") { dojo::CallbackRegistry r; REQUIRE_THROWS(r.subscribe({})); }
DOJO_TEST("callback exception propagates without locking out future operations") {
    dojo::CallbackRegistry r;
    auto t=r.subscribe([](int){throw std::runtime_error("callback failure");});
    REQUIRE_THROWS(r.publish(1));
    REQUIRE(r.unsubscribe(t));
    r.publish(2);
}
DOJO_MAIN()

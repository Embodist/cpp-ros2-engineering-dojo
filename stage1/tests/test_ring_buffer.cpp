#include "dojo/ring_buffer.hpp"
#include "test_harness.hpp"
#include <memory>
DOJO_TEST("full and wrap") { dojo::RingBuffer<int,2> rb; REQUIRE(rb.try_push(1)); REQUIRE(rb.try_push(2)); REQUIRE(!rb.try_push(3)); REQUIRE_EQ(rb.try_pop().value(),1); REQUIRE(rb.try_push(4)); REQUIRE_EQ(rb.try_pop().value(),2); REQUIRE_EQ(rb.try_pop().value(),4); REQUIRE(!rb.try_pop()); }
DOJO_TEST("move only") { dojo::RingBuffer<std::unique_ptr<int>,1> rb; REQUIRE(rb.try_push(std::make_unique<int>(8))); auto x=rb.try_pop(); REQUIRE(x.has_value()); REQUIRE_EQ(**x,8); }
DOJO_MAIN()

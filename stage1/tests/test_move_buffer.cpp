#include "dojo/move_buffer.hpp"
#include "test_harness.hpp"
#include <utility>
DOJO_TEST("zero init and access") { dojo::MoveOnlyBuffer b(4); REQUIRE_EQ(b.size(),4u); REQUIRE_EQ(b.read(3),0u); b.write(3,42); REQUIRE_EQ(b.read(3),42u); REQUIRE_THROWS(b.write(4,3)); }
DOJO_TEST("resize and move") { dojo::MoveOnlyBuffer b(2); b.write(0,9); b.resize(5); REQUIRE_EQ(b.read(0),9u); REQUIRE_EQ(b.read(4),0u); dojo::MoveOnlyBuffer c(std::move(b)); REQUIRE_EQ(b.size(),0u); REQUIRE_EQ(c.read(0),9u); }
DOJO_MAIN()

#include "test_harness.hpp"
DOJO_TEST("smoke: harness runs") { REQUIRE(2 + 2 == 4); }
DOJO_MAIN()

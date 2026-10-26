#include "dojo/moving_average.hpp"
#include "test_harness.hpp"
#include <cmath>
#include <limits>
DOJO_TEST("sliding window") { dojo::MovingAverage m(2); REQUIRE(!m.average()); m.push(2); m.push(4); REQUIRE(std::abs(m.average().value()-3)<1e-12); m.push(8); REQUIRE_EQ(m.count(),2u); REQUIRE(std::abs(m.average().value()-6)<1e-12); }
DOJO_TEST("invalid inputs") { REQUIRE_THROWS(dojo::MovingAverage(0)); dojo::MovingAverage m(3); REQUIRE_THROWS(m.push(std::numeric_limits<double>::infinity())); REQUIRE_THROWS(m.push(std::numeric_limits<double>::quiet_NaN())); }
DOJO_MAIN()

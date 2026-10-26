#include "dojo/parse_result.hpp"
#include "test_harness.hpp"
#include <limits>
#include <variant>
DOJO_TEST("valid measurement") { auto r=dojo::parse_measurement("seq:42;value:-7"); REQUIRE(std::holds_alternative<dojo::Measurement>(r)); REQUIRE_EQ(std::get<dojo::Measurement>(r).seq,42u); REQUIRE_EQ(std::get<dojo::Measurement>(r).value,-7); }
DOJO_TEST("strict format") { REQUIRE(std::holds_alternative<dojo::ParseError>(dojo::parse_measurement("seq:1;value:2 "))); REQUIRE_EQ(std::get<dojo::ParseError>(dojo::parse_measurement("seq:a;value:2")),dojo::ParseError::Format); }
DOJO_TEST("overflow") { auto r=dojo::parse_measurement("seq:4294967296;value:1"); REQUIRE_EQ(std::get<dojo::ParseError>(r),dojo::ParseError::Overflow); }
DOJO_MAIN()

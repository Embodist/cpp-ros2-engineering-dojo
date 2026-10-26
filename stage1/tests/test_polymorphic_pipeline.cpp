#include "dojo/polymorphic_pipeline.hpp"
#include "test_harness.hpp"
#include <memory>
#include <vector>
DOJO_TEST("pipeline order") { dojo::Pipeline p; p.add(std::make_unique<dojo::BiasStage>(2)); p.add(std::make_unique<dojo::ClampStage>(0,3)); std::vector<float> x{-4,0,3}; auto y=p.run(x); REQUIRE_EQ(y.size(),3u); REQUIRE_EQ(y[0],0.0f); REQUIRE_EQ(y[1],2.0f); REQUIRE_EQ(y[2],3.0f); }
DOJO_TEST("empty pipeline identity") { dojo::Pipeline p; std::vector<float> x{1,2}; auto y=p.run(x); REQUIRE(y==x); }
DOJO_TEST("invalid construction") { REQUIRE_THROWS(dojo::ClampStage(5,2)); dojo::Pipeline p; REQUIRE_THROWS(p.add(nullptr)); }
DOJO_MAIN()

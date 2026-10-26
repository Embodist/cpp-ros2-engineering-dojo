#include "dojo/thread_pool.hpp"
#include "test_harness.hpp"
#include <stdexcept>
DOJO_TEST("run and return") { dojo::ThreadPool pool(2); auto a=pool.submit([]{return 6;}); auto b=pool.submit([]{return 7;}); REQUIRE_EQ(a.get()+b.get(),13); pool.shutdown(); }
DOJO_TEST("exception propagation") { dojo::ThreadPool pool(1); auto f=pool.submit([]()->int{throw std::runtime_error("boom");}); REQUIRE_THROWS(f.get()); }
DOJO_TEST("reject after shutdown") { dojo::ThreadPool pool(1); pool.shutdown(); REQUIRE_THROWS(pool.submit([]{return 1;})); }
DOJO_MAIN()

#include "dojo/bounded_queue.hpp"
#include "test_harness.hpp"
#include <future>
#include <stdexcept>
#include <thread>
#include <chrono>
using namespace std::chrono_literals;
DOJO_TEST("FIFO and close") { dojo::BoundedQueue<int> q(2); REQUIRE(q.push(10)); REQUIRE(q.push(11)); q.close(); REQUIRE(!q.push(12)); REQUIRE_EQ(q.pop().value(),10); REQUIRE_EQ(q.pop().value(),11); REQUIRE(!q.pop().has_value()); REQUIRE(q.is_closed()); }
DOJO_TEST("invalid capacity") { REQUIRE_THROWS(dojo::BoundedQueue<int>(0)); }
DOJO_TEST("concurrent single producer") { dojo::BoundedQueue<int> q(1); auto worker=std::async(std::launch::async,[&]{ for(int i=0;i<3;i++) if(!q.push(i)) return false; q.close(); return true; }); for(int i=0;i<3;i++) { auto item=q.pop(); REQUIRE(item.has_value()); REQUIRE_EQ(*item,i); } REQUIRE(worker.wait_for(2s)==std::future_status::ready); REQUIRE(worker.get()); }
DOJO_MAIN()

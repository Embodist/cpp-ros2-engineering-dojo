// 项目 11 参考测试：零分配实时控制环
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <thread>

#include "arena.hpp"          // 项目 6
#include "common/new_counter.hpp"
#include "realtime.hpp"

using namespace std::chrono_literals;

// ============================ PD 控制律正确性 ============================
TEST(Realtime, PdControlLawCorrectness) {
    ControlLoop::InQueue  in;
    ControlLoop::OutQueue out;
    ControlLoop loop(PdGains{2.0, 0.5}, 0.001);
    loop.set_setpoint(10.0);

    in.try_push(SensorSample{8.0, 0});
    ASSERT_TRUE(loop.step(in, out));

    ActuatorCommand c{};
    ASSERT_TRUE(out.try_pop(c));
    // error = 10-8 = 2；deriv = (2-0)/0.001 = 2000；cmd = 2*2 + 0.5*2000 = 1004
    EXPECT_DOUBLE_EQ(c.output, 1004.0);
    EXPECT_FALSE(loop.step(in, out));            // 无新样本 → 不产生指令
}

// ============================ 零分配：运行期 operator new 计数为 0 ============================
TEST(Realtime, ZeroHeapAllocDuringRun) {
    ArenaResource arena(1 << 20);                // 启动期一次性预分配
    double* scratch = static_cast<double*>(arena.allocate(256 * sizeof(double), alignof(double)));

    ControlLoop::InQueue  in;
    ControlLoop::OutQueue out;
    ControlLoop loop(PdGains{1.0, 0.0}, 0.001, scratch, 256);
    loop.set_setpoint(1.0);

    constexpr int kM = 5000;

    NewCounter::reset();
    std::size_t produced = 0;
    for (int i = 0; i < kM; ++i) {
        in.try_push(SensorSample{1.0, static_cast<std::uint64_t>(i)});
        loop.step(in, out);
        ActuatorCommand c{};
        while (out.try_pop(c)) ++produced;       // 及时排空，避免队列满丢指令
    }
    // 训练点：整个运行阶段没有任何 operator new —— 通道无锁、缓冲预分配。
    EXPECT_EQ(NewCounter::allocs(), 0L);
    EXPECT_EQ(produced, static_cast<std::size_t>(kM));
}

// ============================ 对照实验：注入一次 new → 计数上升 ============================
TEST(Realtime, AllocationInjectionShowsUpInCounter) {
    ControlLoop::InQueue  in;
    ControlLoop::OutQueue out;
    ControlLoop loop(PdGains{1.0, 0.0}, 0.001);
    loop.enable_allocation_injection(true);

    for (int i = 0; i < 100; ++i) in.try_push(SensorSample{1.0, static_cast<std::uint64_t>(i)});

    NewCounter::reset();
    for (int i = 0; i < 100; ++i) loop.step(in, out);
    EXPECT_GT(NewCounter::allocs(), 0L);         // 循环里一旦分配，计数立刻反映出来
}

// ============================ 周期抖动统计 p50 / p99 / max ============================
TEST(Realtime, JitterStatistics) {
    ControlLoop::InQueue  in;
    ControlLoop::OutQueue out;
    JitterRecorder rec;
    ControlLoop loop(PdGains{1.0, 0.0}, 0.001);

    constexpr int kCycles = 100;
    for (int i = 0; i < kCycles; ++i) in.try_push(SensorSample{1.0, static_cast<std::uint64_t>(i)});

    loop.run_timed(in, out, kCycles, /*period=*/1ms, rec);
    EXPECT_EQ(rec.size(), static_cast<std::size_t>(kCycles));

    std::printf("[p11] jitter(us): p50=%.1f p99=%.1f max=%.1f\n",
                rec.percentile(0.50), rec.percentile(0.99), rec.max());
    EXPECT_LT(rec.max(), 50000.0);               // 单周期误差 < 50ms（宽松上界，仅为健全性检查）
}

// ============================ 三线程流水线：传感器 → 控制 → 执行器 ============================
TEST(Realtime, ThreadedPipeline) {
    ControlLoop::InQueue  in;
    ControlLoop::OutQueue out;
    ControlLoop loop(PdGains{1.0, 0.0}, 0.001);
    loop.set_setpoint(5.0);

    constexpr int kM = 100000;
    std::atomic<int> produced{0};

    std::thread sensor([&] {
        for (int i = 0; i < kM;) if (in.try_push(SensorSample{1.0, static_cast<std::uint64_t>(i)})) ++i;
    });
    std::thread actuator([&] {
        ActuatorCommand c{};
        while (produced.load() < kM) if (out.try_pop(c)) produced.fetch_add(1);
    });

    for (int i = 0; i < kM;) if (loop.step(in, out)) ++i;

    sensor.join();
    actuator.join();
    EXPECT_EQ(produced.load(), kM);              // 每个样本都产出并消费了一条指令
}

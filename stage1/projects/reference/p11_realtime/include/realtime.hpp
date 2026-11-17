#pragma once
// 零分配实时控制环（项目 11）—— 组合项目 6（Arena/池）+ 项目 9（SPSC 无锁队列）
// ============================================================================
// 训练点：运行期零堆分配、零锁；用 steady_clock + sleep_until 驱动固定周期；测量周期抖动。
//
// 数据流：传感器线程 --(SPSC)--> 控制环线程 --(SPSC)--> 执行器线程
//   * 通道全部是无锁 SPSC（项目 9），跨线程传递不需要加锁。
//   * 控制环里【不出现】任何会分配内存的东西：容器扩容、std::string、std::function、
//     iostream 统统避开；需要暂存就写进启动期从 Arena 预分配的缓冲。
//   * 控制步 step()/run 全程 noexcept —— 控制路径不许抛异常。
// ============================================================================

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <thread>

#include "spsc_queue.hpp"   // 项目 9

using Clock = std::chrono::steady_clock;

struct SensorSample   { double value; std::uint64_t seq; };
struct ActuatorCommand { double output; };
struct PdGains        { double kp; double kd; };

// 抖动采集：定长数组 + 就地统计（不分配内存，可安全用于实时线程）。
class JitterRecorder {
public:
    static constexpr std::size_t kMax = 8192;

    void record(double us) noexcept { if (n_ < kMax) samples_[n_++] = us; }
    std::size_t size() const noexcept { return n_; }

    double max() const noexcept {
        double m = 0.0;
        for (std::size_t i = 0; i < n_; ++i) m = std::max(m, std::abs(samples_[i]));
        return m;
    }
    // p ∈ [0,1]，返回 |偏差| 的分位数。
    double percentile(double p) noexcept {
        if (n_ == 0) return 0.0;
        std::array<double, kMax> tmp{};
        for (std::size_t i = 0; i < n_; ++i) tmp[i] = std::abs(samples_[i]);
        std::sort(tmp.begin(), tmp.begin() + static_cast<std::ptrdiff_t>(n_));
        const auto idx = static_cast<std::size_t>(p * static_cast<double>(n_ - 1));
        return tmp[idx];
    }

private:
    std::array<double, kMax> samples_{};
    std::size_t              n_ = 0;
};

class ControlLoop {
public:
    static constexpr std::size_t kQueueCapacity = 256;
    using InQueue  = SpscQueue<SensorSample, kQueueCapacity>;
    using OutQueue = SpscQueue<ActuatorCommand, kQueueCapacity>;

    // scratch/scratch_cap：启动期从 Arena 预分配的命令日志缓冲（循环里写入它，不再分配）。
    ControlLoop(PdGains g, double dt, double* scratch = nullptr, std::size_t scratch_cap = 0) noexcept
        : gains_(g), dt_(dt), scratch_(scratch), scratch_cap_(scratch_cap) {}

    void set_setpoint(double sp) noexcept { setpoint_ = sp; }
    void enable_allocation_injection(bool on) noexcept { inject_alloc_ = on; }
    std::size_t cycles() const noexcept { return cycle_; }

    // 单步：取一个传感器样本 → PD 控制 → 写指令。零锁、零分配、noexcept。
    bool step(InQueue& in, OutQueue& out) noexcept {
        // 背压：输出队列满时不消费输入，否则样本会被丢掉，生产者/消费者会活锁。
        if (has_pending_) {
            if (!out.try_push(pending_)) return false;
            has_pending_ = false;
            return true;
        }
        if (out.size_approx() >= OutQueue::capacity()) return false;

        SensorSample s;
        if (!in.try_pop(s)) return false;          // 无新数据

        const double error = setpoint_ - s.value;
        const double deriv = (error - prev_error_) / dt_;
        prev_error_ = error;
        const double cmd = gains_.kp * error + gains_.kd * deriv;

        if (scratch_ != nullptr && scratch_cap_ != 0)
            scratch_[cycle_ % scratch_cap_] = cmd; // 写预分配缓冲，无分配

        if (inject_alloc_) {                       // 对照实验：故意分配一次
            double* p = new double(cmd);
            delete p;
        }

        ++cycle_;
        ActuatorCommand command{cmd};
        if (!out.try_push(command)) {              // 竞态下仍可能满：留到下一次重试
            pending_ = command;
            has_pending_ = true;
            return false;
        }
        return true;
    }

    // 定时驱动：每 period 跑一步，把"唤醒时刻相对理想时刻"的偏差（微秒）记进 rec。
    void run_timed(InQueue& in, OutQueue& out, std::size_t cycles,
                   Clock::duration period, JitterRecorder& rec) noexcept {
        auto next = Clock::now();
        for (std::size_t i = 0; i < cycles; ++i) {
            next += period;
            std::this_thread::sleep_until(next);
            const auto now = Clock::now();
            rec.record(std::chrono::duration<double, std::micro>(now - next).count());
            step(in, out);
        }
    }

private:
    PdGains     gains_;
    double      dt_;
    double      setpoint_    = 0.0;
    double      prev_error_  = 0.0;
    double*     scratch_;
    std::size_t scratch_cap_;
    std::size_t cycle_       = 0;
    ActuatorCommand pending_{};
    bool        has_pending_ = false;
    bool        inject_alloc_ = false;
};

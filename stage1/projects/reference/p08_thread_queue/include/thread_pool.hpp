#pragma once
// ThreadPool —— 固定线程数线程池（项目 8）
// ============================================================================
// 训练点：线程池的任务队列、优雅关闭、异常经 future 传回、std::invoke_result 推导返回类型。
//
// 关闭语义（写死并测试）：析构 = 优雅关闭：先 close() 队列（不再收新任务），再 join 所有 worker，
//   worker 会把【已在队列里的任务全部跑完】才退出。任务不丢，也不死锁。
//
// 任务类型用项目 7 的 MoveOnlyFunction —— 队列里只移动不拷贝。
// ============================================================================

#include <cstddef>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "blocking_queue.hpp"
#include "function.hpp"

class ThreadPool {
public:
    explicit ThreadPool(std::size_t threads) {
        if (threads == 0) threads = 1;
        workers_.reserve(threads);
        for (std::size_t i = 0; i < threads; ++i)
            workers_.emplace_back([this] { worker_loop(); });
    }

    ~ThreadPool() {
        tasks_.close();                               // 不再收新任务
        for (auto& w : workers_)
            if (w.joinable()) w.join();               // 等 worker 把剩余任务跑完
    }

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // 提交任务：返回 future，结果/异常都通过它传出。
    template <class F, class... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<std::invoke_result_t<F, Args...>> {
        using R = std::invoke_result_t<F, Args...>;
        // packaged_task 负责"捕获异常 → 塞进 future"。用 shared_ptr 装箱是因为
        // packaged_task 不可拷贝，而我们队列里存的是可移动的 MoveOnlyFunction。
        auto task = std::make_shared<std::packaged_task<R()>>(
            [f = std::forward<F>(f), ...a = std::forward<Args>(args)]() mutable -> R {
                return std::invoke(std::move(f), std::move(a)...);
            });
        std::future<R> fut = task->get_future();
        // 易错点: 必须在把 task 交给队列【之前】取 future；一旦移走，task 就不可寻址了。
        tasks_.push(MoveOnlyFunction<void()>([task] { (*task)(); }));
        return fut;
    }

    std::size_t thread_count() const noexcept { return workers_.size(); }

private:
    void worker_loop() {
        MoveOnlyFunction<void()> task;
        while (tasks_.pop(task)) {                    // 取到即执行；关闭且取空 → 退出
            try {
                task();
            } catch (...) {
                // 易错点: 任务本体的异常已被 packaged_task 捕获进 future，正常不会到这。
                //         这里兜底防止"未包装的异常"把 worker 线程打死（否则池会慢慢枯竭）。
            }
        }
    }

    BlockingQueue<MoveOnlyFunction<void()>> tasks_;
    std::vector<std::thread>                workers_;
};

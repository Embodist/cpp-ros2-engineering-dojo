#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
namespace dojo {
class ThreadPool {
public:
    explicit ThreadPool(std::size_t workers); // TODO workers>0
    ThreadPool(const ThreadPool&)=delete;
    ThreadPool& operator=(const ThreadPool&)=delete;
    ~ThreadPool();
    std::future<int> submit(std::function<int()> job); // TODO
    void shutdown() noexcept; // TODO 接收中的任务执行完；不可在自身工作线程调用（记录此前置条件）
private:
    void run_worker();
    std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<std::packaged_task<int()>> jobs_;
    std::vector<std::thread> workers_;
    bool stopping_=false;
};
inline ThreadPool::ThreadPool(std::size_t workers) {
    if (workers == 0) throw std::invalid_argument("workers");
    workers_.reserve(workers);
    try {
        for (std::size_t i = 0; i < workers; ++i) {
            workers_.emplace_back(&ThreadPool::run_worker, this);
        }
    } catch (...) {
        shutdown();
        throw;
    }
}

inline ThreadPool::~ThreadPool() {
    shutdown();
}

inline std::future<int> ThreadPool::submit(std::function<int()> job) {
    std::packaged_task<int()> task(std::move(job));
    std::future<int> result = task.get_future();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (stopping_) throw std::runtime_error("submit after shutdown");
        jobs_.push(std::move(task));
    }
    cv_.notify_one();
    return result;
}

inline void ThreadPool::shutdown() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
        if (worker.joinable()) worker.join();
    }
}

inline void ThreadPool::run_worker() {
    for (;;) {
        std::packaged_task<int()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] { return stopping_ || !jobs_.empty(); });
            if (jobs_.empty()) {
                if (stopping_) return;
                continue;
            }
            task = std::move(jobs_.front());
            jobs_.pop();
        }
        task();
    }
}

}

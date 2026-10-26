#pragma once
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
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
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline ThreadPool::ThreadPool(std::size_t workers) { (void)workers; /* TODO */ }
inline ThreadPool::~ThreadPool() { /* TODO */ }
inline std::future<int> ThreadPool::submit(std::function<int()> job) { (void)job; throw std::runtime_error("TODO"); }
inline void ThreadPool::shutdown() noexcept { /* TODO */ }
inline void ThreadPool::run_worker() { /* TODO */ }

}

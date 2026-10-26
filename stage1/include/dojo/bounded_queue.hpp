#pragma once
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <utility>
namespace dojo {
template<class T> class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : capacity_(capacity) { if(capacity==0) throw std::invalid_argument("capacity"); }
    BoundedQueue(const BoundedQueue&)=delete;
    BoundedQueue& operator=(const BoundedQueue&)=delete;
    bool push(T value);           // TODO: 阻塞直到有空间或 close；成功返回 true
    std::optional<T> pop();       // TODO: 阻塞直到有数据或 close+排空
    void close() noexcept;        // TODO: 幂等，notify_all
    bool is_closed() const;       // TODO: 线程安全
private:
    std::size_t capacity_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
    std::deque<T> q_;
    bool closed_ = false;
};
// TODO: 模板定义必须可见；禁止空转 busy waiting。
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
template<class T> bool BoundedQueue<T>::push(T value) { (void)value; return false; /* TODO */ }
template<class T> std::optional<T> BoundedQueue<T>::pop() { return std::nullopt; /* TODO */ }
template<class T> void BoundedQueue<T>::close() noexcept { /* TODO */ }
template<class T> bool BoundedQueue<T>::is_closed() const { return false; /* TODO */ }

}

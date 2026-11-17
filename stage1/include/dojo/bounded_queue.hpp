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
template<class T> bool BoundedQueue<T>::push(T value) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_full_.wait(lock, [this] { return closed_ || q_.size() < capacity_; });
    if (closed_) return false;
    q_.push_back(std::move(value));
    not_empty_.notify_one();
    return true;
}

template<class T> std::optional<T> BoundedQueue<T>::pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] { return closed_ || !q_.empty(); });
    if (q_.empty()) return std::nullopt;
    T value = std::move(q_.front());
    q_.pop_front();
    not_full_.notify_one();
    return value;
}

template<class T> void BoundedQueue<T>::close() noexcept {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    not_full_.notify_all();
    not_empty_.notify_all();
}

template<class T> bool BoundedQueue<T>::is_closed() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return closed_;
}

}

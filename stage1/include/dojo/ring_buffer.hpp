#pragma once
#include <array>
#include <cstddef>
#include <optional>
#include <utility>
namespace dojo {
template<class T, std::size_t N> class RingBuffer {
    static_assert(N>0);
public:
    bool try_push(T value);    // TODO 满返回 false；保持 value 移动语义
    std::optional<T> try_pop(); // TODO
    std::size_t size() const noexcept { return size_; }
    static constexpr std::size_t capacity() noexcept { return N; }
private:
    std::array<std::optional<T>,N> items_{};
    std::size_t head_=0; // 下一出队位置
    std::size_t tail_=0; // 下一入队位置
    std::size_t size_=0;
};
template<class T,std::size_t N> bool RingBuffer<T,N>::try_push(T value) {
    if (size_ == N) return false;
    items_[tail_].emplace(std::move(value));
    tail_ = (tail_ + 1) % N;
    ++size_;
    return true;
}

template<class T,std::size_t N> std::optional<T> RingBuffer<T,N>::try_pop() {
    if (size_ == 0) return std::nullopt;
    std::optional<T> out = std::move(items_[head_]);
    items_[head_].reset();
    head_ = (head_ + 1) % N;
    --size_;
    return out;
}

}

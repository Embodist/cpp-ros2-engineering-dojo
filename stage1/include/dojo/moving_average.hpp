#pragma once
#include <cmath>
#include <cstddef>
#include <deque>
#include <optional>
#include <stdexcept>
namespace dojo {
class MovingAverage {
public:
    explicit MovingAverage(std::size_t window); // TODO: reject zero
    void push(double value);    // TODO: reject !isfinite
    std::optional<double> average() const noexcept; // TODO
    std::size_t count() const noexcept { return values_.size(); }
private:
    std::size_t window_=0;
    std::deque<double> values_;
    double sum_=0;
    std::size_t since_recalc_=0;
};
inline MovingAverage::MovingAverage(std::size_t window) : window_(window) {
    if (window == 0) throw std::invalid_argument("window");
}

inline void MovingAverage::push(double value) {
    if (!std::isfinite(value)) throw std::invalid_argument("non-finite");
    values_.push_back(value);
    sum_ += value;
    if (values_.size() > window_) {
        sum_ -= values_.front();
        values_.pop_front();
    }
    if (++since_recalc_ >= window_) {
        sum_ = 0;
        for (double v : values_) sum_ += v;
        since_recalc_ = 0;
    }
}

inline std::optional<double> MovingAverage::average() const noexcept {
    if (values_.empty()) return std::nullopt;
    return sum_ / static_cast<double>(values_.size());
}

}

#pragma once
#include <cstddef>
#include <deque>
#include <optional>
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
};
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline MovingAverage::MovingAverage(std::size_t window) : window_(window) { /* TODO: validate */ }
inline void MovingAverage::push(double value) { (void)value; /* TODO */ }
inline std::optional<double> MovingAverage::average() const noexcept { return std::nullopt; /* TODO */ }

}

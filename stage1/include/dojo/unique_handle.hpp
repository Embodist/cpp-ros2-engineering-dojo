#pragma once
#include <functional>
#include <utility>
namespace dojo {
class UniqueHandle {
public:
    using Closer = std::function<void(int)>;
    explicit UniqueHandle(int handle, Closer closer) : value_(handle), closer_(std::move(closer)) {}
    UniqueHandle(const UniqueHandle&) = delete;
    UniqueHandle& operator=(const UniqueHandle&) = delete;
    UniqueHandle(UniqueHandle&& other) noexcept;            // TODO
    UniqueHandle& operator=(UniqueHandle&& other) noexcept; // TODO
    ~UniqueHandle() noexcept;                                // TODO: closer 不得抛异常
    int get() const noexcept { return value_; }
    bool valid() const noexcept { return value_ >= 0; }
    int release() noexcept;                // TODO
    void reset(int value = -1) noexcept;   // TODO
private:
    int value_ = -1;
    Closer closer_;
};
inline UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept
    : value_(other.value_), closer_(std::move(other.closer_)) {
    other.value_ = -1;
}

inline UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept {
    if (this != &other) {
        if (value_ >= 0 && closer_) {
            closer_(value_);
        }
        value_ = other.value_;
        closer_ = std::move(other.closer_);
        other.value_ = -1;
    }
    return *this;
}

inline UniqueHandle::~UniqueHandle() noexcept {
    if (value_ >= 0 && closer_) {
        closer_(value_);
    }
}

inline int UniqueHandle::release() noexcept {
    const int old = value_;
    value_ = -1;
    return old;
}

inline void UniqueHandle::reset(int value) noexcept {
    if (value == value_) {
        return;
    }
    if (value_ >= 0 && closer_) {
        closer_(value_);
    }
    value_ = value;
}

}

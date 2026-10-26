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
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept : value_(-1) { (void)other; /* TODO */ }
inline UniqueHandle& UniqueHandle::operator=(UniqueHandle&& other) noexcept { (void)other; return *this; /* TODO */ }
inline UniqueHandle::~UniqueHandle() noexcept { /* TODO */ }
inline int UniqueHandle::release() noexcept { return -1; /* TODO */ }
inline void UniqueHandle::reset(int value) noexcept { (void)value; /* TODO */ }

}

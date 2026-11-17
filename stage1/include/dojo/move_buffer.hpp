#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace dojo {
class MoveOnlyBuffer {
public:
    explicit MoveOnlyBuffer(std::size_t size);       // TODO
    MoveOnlyBuffer(const MoveOnlyBuffer&) = delete;
    MoveOnlyBuffer& operator=(const MoveOnlyBuffer&) = delete;
    MoveOnlyBuffer(MoveOnlyBuffer&& other) noexcept;  // TODO
    MoveOnlyBuffer& operator=(MoveOnlyBuffer&& other) noexcept; // TODO
    void resize(std::size_t new_size);              // TODO
    std::uint8_t read(std::size_t index) const;     // TODO
    void write(std::size_t index,std::uint8_t value); // TODO
    std::uint8_t* data() noexcept { return data_.get(); }
    const std::uint8_t* data() const noexcept { return data_.get(); }
    std::size_t size() const noexcept { return size_; }
private:
    std::unique_ptr<std::uint8_t[]> data_;
    std::size_t size_ = 0;
};
inline MoveOnlyBuffer::MoveOnlyBuffer(std::size_t n)
    : data_(std::make_unique<std::uint8_t[]>(n)), size_(n) {}

inline MoveOnlyBuffer::MoveOnlyBuffer(MoveOnlyBuffer&& other) noexcept
    : data_(std::move(other.data_)), size_(other.size_) {
    other.size_ = 0;
}

inline MoveOnlyBuffer& MoveOnlyBuffer::operator=(MoveOnlyBuffer&& other) noexcept {
    if (this != &other) {
        data_ = std::move(other.data_);
        size_ = other.size_;
        other.size_ = 0;
    }
    return *this;
}

inline void MoveOnlyBuffer::resize(std::size_t n) {
    auto new_data = std::make_unique<std::uint8_t[]>(n);
    const std::size_t copy_n = n < size_ ? n : size_;
    for (std::size_t i = 0; i < copy_n; ++i) {
        new_data[i] = data_[i];
    }
    data_ = std::move(new_data);
    size_ = n;
}

inline std::uint8_t MoveOnlyBuffer::read(std::size_t index) const {
    if (index >= size_) throw std::out_of_range("index");
    return data_[index];
}

inline void MoveOnlyBuffer::write(std::size_t index, std::uint8_t value) {
    if (index >= size_) throw std::out_of_range("index");
    data_[index] = value;
}

}

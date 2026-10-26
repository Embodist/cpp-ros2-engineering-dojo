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
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline MoveOnlyBuffer::MoveOnlyBuffer(std::size_t n) { (void)n; /* TODO */ }
inline MoveOnlyBuffer::MoveOnlyBuffer(MoveOnlyBuffer&& other) noexcept { (void)other; /* TODO */ }
inline MoveOnlyBuffer& MoveOnlyBuffer::operator=(MoveOnlyBuffer&& other) noexcept { (void)other; return *this; /* TODO */ }
inline void MoveOnlyBuffer::resize(std::size_t n) { (void)n; /* TODO */ }
inline std::uint8_t MoveOnlyBuffer::read(std::size_t index) const { (void)index; throw std::out_of_range("TODO"); }
inline void MoveOnlyBuffer::write(std::size_t index, std::uint8_t value) { (void)index; (void)value; /* TODO */ }

}

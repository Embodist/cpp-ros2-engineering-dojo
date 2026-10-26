#pragma once
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace dojo {
template <class T>
class MiniVector {
public:
    MiniVector() = default;
    MiniVector(const MiniVector& other);            // TODO: 深拷贝
    MiniVector& operator=(const MiniVector& other); // TODO
    MiniVector(MiniVector&& other) noexcept;         // TODO
    MiniVector& operator=(MiniVector&& other) noexcept; // TODO
    ~MiniVector();                                  // TODO: destroy + deallocate
    void reserve(std::size_t new_capacity);         // TODO: 强异常保证（适用时）
    void push_back(const T& value);                 // TODO
    void push_back(T&& value);                      // TODO
    T& at(std::size_t index) { if(index >= size_) throw std::out_of_range("index"); return data_[index]; }
    const T& at(std::size_t index) const { if(index >= size_) throw std::out_of_range("index"); return data_[index]; }
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
private:
    std::allocator<T> alloc_{};
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};
// TODO: 把模板成员定义写在本头文件末尾，勿只写到 .cpp。
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
template<class T> MiniVector<T>::MiniVector(const MiniVector& other) { (void)other; /* TODO */ }
template<class T> MiniVector<T>& MiniVector<T>::operator=(const MiniVector& other) { (void)other; return *this; /* TODO */ }
template<class T> MiniVector<T>::MiniVector(MiniVector&& other) noexcept { (void)other; /* TODO */ }
template<class T> MiniVector<T>& MiniVector<T>::operator=(MiniVector&& other) noexcept { (void)other; return *this; /* TODO */ }
template<class T> MiniVector<T>::~MiniVector() { /* TODO */ }
template<class T> void MiniVector<T>::reserve(std::size_t n) { (void)n; /* TODO */ }
template<class T> void MiniVector<T>::push_back(const T& value) { (void)value; /* TODO */ }
template<class T> void MiniVector<T>::push_back(T&& value) { (void)value; /* TODO */ }

}

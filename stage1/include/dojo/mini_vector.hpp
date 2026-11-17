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
    MiniVector(const MiniVector& other);
    MiniVector& operator=(const MiniVector& other);
    MiniVector(MiniVector&& other) noexcept;
    MiniVector& operator=(MiniVector&& other) noexcept;
    ~MiniVector();
    void reserve(std::size_t new_capacity);
    void push_back(const T& value);
    void push_back(T&& value);
    T& at(std::size_t index) { if(index >= size_) throw std::out_of_range("index"); return data_[index]; }
    const T& at(std::size_t index) const { if(index >= size_) throw std::out_of_range("index"); return data_[index]; }
    std::size_t size() const noexcept { return size_; }
    std::size_t capacity() const noexcept { return capacity_; }
private:
    void destroy_all() noexcept;
    void swap(MiniVector& other) noexcept;

    std::allocator<T> alloc_{};
    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

template <class T>
MiniVector<T>::MiniVector(const MiniVector& other)
    : size_(other.size_), capacity_(other.capacity_) {
    if (capacity_ == 0) return;
    data_ = alloc_.allocate(capacity_);
    std::size_t constructed = 0;
    try {
        for (; constructed < size_; ++constructed) {
            std::construct_at(data_ + constructed, other.data_[constructed]);
        }
    } catch (...) {
        for (std::size_t i = 0; i < constructed; ++i) {
            std::destroy_at(data_ + i);
        }
        alloc_.deallocate(data_, capacity_);
        throw;
    }
}

template <class T>
MiniVector<T>& MiniVector<T>::operator=(const MiniVector& other) {
    if (this != &other) {
        MiniVector tmp(other);
        swap(tmp);
    }
    return *this;
}

template <class T>
MiniVector<T>::MiniVector(MiniVector&& other) noexcept
    : data_(other.data_), size_(other.size_), capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}

template <class T>
MiniVector<T>& MiniVector<T>::operator=(MiniVector&& other) noexcept {
    if (this != &other) {
        destroy_all();
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }
    return *this;
}

template <class T>
MiniVector<T>::~MiniVector() {
    destroy_all();
}

template <class T>
void MiniVector<T>::reserve(std::size_t new_capacity) {
    if (new_capacity <= capacity_) return;

    T* new_data = alloc_.allocate(new_capacity);
    std::size_t constructed = 0;
    try {
        for (; constructed < size_; ++constructed) {
            std::construct_at(new_data + constructed,
                              std::move_if_noexcept(data_[constructed]));
        }
    } catch (...) {
        for (std::size_t i = 0; i < constructed; ++i) {
            std::destroy_at(new_data + i);
        }
        alloc_.deallocate(new_data, new_capacity);
        throw;
    }

    for (std::size_t i = 0; i < size_; ++i) {
        std::destroy_at(data_ + i);
    }
    if (data_) alloc_.deallocate(data_, capacity_);
    data_ = new_data;
    capacity_ = new_capacity;
}

template <class T>
void MiniVector<T>::push_back(const T& value) {
    if (size_ == capacity_) {
        T tmp(value);
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        std::construct_at(data_ + size_, std::move_if_noexcept(tmp));
    } else {
        std::construct_at(data_ + size_, value);
    }
    ++size_;
}

template <class T>
void MiniVector<T>::push_back(T&& value) {
    if (size_ == capacity_) {
        T tmp(std::move(value));
        reserve(capacity_ == 0 ? 1 : capacity_ * 2);
        std::construct_at(data_ + size_, std::move(tmp));
    } else {
        std::construct_at(data_ + size_, std::move(value));
    }
    ++size_;
}

template <class T>
void MiniVector<T>::destroy_all() noexcept {
    for (std::size_t i = 0; i < size_; ++i) {
        std::destroy_at(data_ + i);
    }
    if (data_) alloc_.deallocate(data_, capacity_);
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

template <class T>
void MiniVector<T>::swap(MiniVector& other) noexcept {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
}

}

#include <cstddef>
#include <algorithm>
#include <memory>
#include <iostream>

template <typename T>
class Vector {
private:
    struct RawMemory{
        T* buf = nullptr;
        size_t capacity_ = 0;

        static T* Allocate(size_t n) {
            return reinterpret_cast<T*>(operator new(n * sizeof(T)));
        }

        static void Deallocate(T* buf) {
            operator delete (buf);
        }

        RawMemory() = default;

        RawMemory(size_t n) {
            buf = Allocate(n);
            capacity_ = n;
        }

        RawMemory(const RawMemory&) = delete;

        RawMemory(RawMemory&& other) {
            swap(other);
        }

        RawMemory& operator=(const RawMemory&) = delete;

        RawMemory& operator=(RawMemory&& other) {
            swap(other);
            return *this;
        }

        ~RawMemory() {
            Deallocate(buf);
        }

        T* operator + (size_t i) const {
            return (buf + i);
        }

        T& operator[](size_t i) {
            return buf[i];
        }

        const T& operator[](size_t i) const {
            return buf[i];
        }

        void swap(RawMemory& other) {
            std::swap(buf, other.buf);
            std::swap(capacity_, other.capacity_);
        }
    };

    RawMemory data_;
    size_t  size_ = 0;

public:
    Vector() = default;

    Vector(Vector&& other) {
        Swap(other);
    }

    explicit Vector(size_t n): data_(n) {
        std::uninitialized_value_construct_n(data_.buf, n);
        size_ = n;
    }

    explicit Vector(const Vector& other): data_(other.size_) {
        std::uninitialized_copy_n(other.data_.buf, other.size_, data_.buf);
        size_ = other.size_;
    }

    Vector& operator=(const Vector& other) {
        if (other.size_ > data_.capacity_) {
            Vector tmp(other);
            swap(tmp);
        } else {
          for (size_t i = 0; i < std::min(size_, other.size_); ++i) {
              data_[i] = other[i];
          }
          if (size_ < other.size_) {
              std::uninitialized_copy_n(other.data_.buf + size_,
                                        other.size_ - size_, data_.buf + size_);
          } else if (size_ > other.size_) {
              std::destroy_n(data_.buf + other.size_, size_ - other.size_);
          }
          size_ = other.size_;
        }
        return *this;
    }

    Vector& operator=(Vector&& other) {
        Swap(other);
        return *this;
    }

    ~Vector() {
        std::destroy_n(data_.buf, size_);
    }

    void reserve(size_t n) {
        if (n > data_.capacity_) {
            RawMemory new_data(n);
            std::uninitialized_move_n(data_.buf, size_, new_data.buf);
            std::destroy_n(data_.buf, size_);
            data_.swap(new_data);
        }
    }

    void resize(size_t n) {
        reserve(n);
        if (size_ < n) {
            std::uninitialized_value_construct_n(data_ + size_, n - size_);
        } else if (size_ > n) {
            std::destroy_n(data_ + n, size_ - n);
        }
        size_ = n;
    }

    void push_back(const T& elem) {
        if (size_ == data_.capacity_) {
            reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new (data_ + size_) T(elem);
        ++size_;
    }

    void push_back(const T&& elem) {
        if (size_ == data_.capacity_) {
            reserve(size_ == 0 ? 1 : size_ * 2);
        }
        new (data_ + size_) T(std::move(elem));
        ++size_;
    }

    void pop_back() {
        std::destroy_at(data_ + size_ - 1);
        --size_;
    }

    void swap(Vector& other) {
        data_.swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return data_.capacity_;
    }

    const T& operator[](size_t idx) const {
        return data_[idx];
    }

    T& operator[](size_t idx) {
        return data_[idx];
    }

    T* begin() const {
        return data_.buf;
    }

    T* end() const {
        return data_ + size_;
    }
    
    void clear() {
        std::destroy_n(data_.buf, size_);
        size_ = 0;
    }
};

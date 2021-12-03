#pragma once

#include "sw_fwd.h"  // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <utility>

class EnableSharedFromThisBase {};
// Look for usage examples in tests
template <typename T>
class EnableSharedFromThis : public EnableSharedFromThisBase {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(weak_);
    }
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<const T>(weak_);
    }

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(weak_);
    }

    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(weak_);
    }

    WeakPtr<T> weak_;
};


class ControlBlockBase {
public:
    size_t ref_counter = 0;
    size_t weak_counter = 0;

    virtual ~ControlBlockBase() = default;

    virtual void MyFunc() {}
};

template <typename U>
class ControlBlockPointer : public ControlBlockBase {
public:
    U* ptr = nullptr;

    ControlBlockPointer(U* pointer) : ptr(pointer) {
    }

    ~ControlBlockPointer() override {
        if constexpr (std::is_convertible_v<U*, EnableSharedFromThisBase*>) {
            if (ptr) {
                ptr->weak_.ptr_ = nullptr;
                ptr->weak_.block_ = nullptr;
            }
        }
        delete ptr;
    }

    void MyFunc() override {
        if constexpr (std::is_convertible_v<U*, EnableSharedFromThisBase*>) {
            ptr->weak_.ptr_ = nullptr;
            ptr->weak_.block_ = nullptr;
        }
        delete ptr;
        ptr = nullptr;
    }
};

template <typename U>
class ControlBlockHolder : public ControlBlockBase {
public:
    template <typename... Args>
    ControlBlockHolder(Args&&... args) {
        new (&storage_) U(std::forward<Args>(args)...);
    }

    U* GetRawPtr() {
        return reinterpret_cast<U*>(&storage_);
    }

    ~ControlBlockHolder() {
        if constexpr (std::is_convertible_v<U*, EnableSharedFromThisBase*>) {
            GetRawPtr()->weak_.ptr_ = nullptr;
            GetRawPtr()->weak_.block_ = nullptr;
        }

        if (flag) {
            GetRawPtr()->~U();
        }
    }

    std::aligned_storage_t<sizeof(U), alignof(U)> storage_;

    void MyFunc() override {
        if constexpr (std::is_convertible_v<U*, EnableSharedFromThisBase*>) {
            GetRawPtr()->weak_.ptr_ = nullptr;
            GetRawPtr()->weak_.block_ = nullptr;
        }
        GetRawPtr()->~U();
        flag = false;
    }

    bool flag = true;
};

// https://en.cppreference.com/w/cpp/memory/shared_ptr
template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr(){};
    SharedPtr(std::nullptr_t){};
    explicit SharedPtr(T* ptr) : ptr_(ptr), block_(new ControlBlockPointer<T>(ptr)) {
        block_->ref_counter = 1;

        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            if (ptr->weak_) {
                Reset();
                ptr_ = static_cast<T*>(ptr);
                block_ = ptr_->weak_.block_;
                if (block_) {
                    ++block_->ref_counter;
                }
            } else {
                ptr->weak_.block_ = block_;
                ptr->weak_.ptr_ = ptr_;
            }
        }
    }

    template <class U>
    explicit SharedPtr(U* ptr)
        : ptr_(static_cast<T*>(ptr)), block_(new ControlBlockPointer<U>(ptr)) {
        block_->ref_counter = 1;

        if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
            if (ptr->weak_) {
                Reset();
                ptr_ = static_cast<T*>(ptr);
                block_ = ptr_->weak_.block_;
                if (block_) {
                    ++block_->ref_counter;
                }
            } else {
                ptr->weak_.block_ = block_;
                ptr->weak_.ptr_ = ptr_;
            }
        }
    }

    SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), block_(other.block_) {
        if (block_) {
            ++block_->ref_counter;
        }
    }
    SharedPtr(SharedPtr&& other) : ptr_(other.ptr_), block_(other.block_) {
        other.ptr_ = nullptr;
        other.block_ = nullptr;
    }

    template <class U>
    SharedPtr(const SharedPtr<U>& other) : ptr_(other.Get()), block_(other.GetBlock()) {
        if (block_) {
            ++block_->ref_counter;
        }
    }
    template <class U>
    SharedPtr(SharedPtr<U>&& other) : ptr_(other.Get()), block_(other.GetBlock()) {
        if (block_) {
            ++block_->ref_counter;
        }
        other.Delete();
    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) : ptr_(ptr), block_(other.GetBlock()) {
        if (block_) {
            ++block_->ref_counter;
        }
    }

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.Expired()) {
            throw BadWeakPtr();
        }

        ptr_ = other.ptr_;
        block_ = other.block_;
        if (block_) {
            ++block_->ref_counter;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (other != *this) {
            Delete();
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                ++block_->ref_counter;
            }
        }
        return *this;
    }
    SharedPtr& operator=(SharedPtr&& other) {
        if (other != *this) {
            Delete();
            ptr_ = other.ptr_;
            block_ = other.block_;
            other.ptr_ = nullptr;
            other.block_ = nullptr;
        }
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        Delete();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        Delete();
    }
    void Reset(T* ptr) {
        Delete();
        ptr_ = ptr;
        block_ = new ControlBlockPointer<T>(ptr);
        block_->ref_counter = 1;
    }

    template <class U>
    void Reset(U* ptr) {
        Delete();
        ptr_ = ptr;
        block_ = new ControlBlockPointer<U>(ptr);
        block_->ref_counter = 1;
    }
    void Swap(SharedPtr& other) {
        std::swap(ptr_, other.ptr_);
        std::swap(block_, other.block_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return ptr_;
    }
    ControlBlockBase* GetBlock() const {
        return block_;
    }
    T& operator*() const {
        return *ptr_;
    }
    T* operator->() const {
        return ptr_;
    }
    size_t UseCount() const {
        if (block_) {
            return block_->ref_counter;
        } else {
            return 0;
        }
    }
    explicit operator bool() const {
        return (ptr_ != nullptr);
    }

    void Delete() {
        if (block_) {
            if (block_->ref_counter <= 1 && block_->weak_counter == 0) {
                delete block_;
            } else {
                --block_->ref_counter;
                if (block_->weak_counter > 0 && block_->ref_counter == 0) {
                    block_->MyFunc();
                }
            }
        }
        ptr_ = nullptr;
        block_ = nullptr;
    }

private:
    T* ptr_ = nullptr;
    ControlBlockBase* block_ = nullptr;

    template <typename U, typename... Args>
    friend SharedPtr<U> MakeShared(Args&&... args);

    template <typename F, typename U>
    inline friend bool operator==(const SharedPtr<F>& left, const SharedPtr<U>& right);

    friend WeakPtr<T>;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    return (left.ptr_ == right.ptr_ && left.block_ == right.block_);
}

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    SharedPtr<T> sp;

    auto block = new ControlBlockHolder<T>(std::forward<Args>(args)...);
    sp.block_ = block;
    sp.block_->ref_counter = 1;
    sp.ptr_ = block->GetRawPtr();

    if constexpr (std::is_convertible_v<T*, EnableSharedFromThisBase*>) {
        sp.ptr_->weak_.block_ = sp.block_;
        sp.ptr_->weak_.ptr_ = sp.ptr_;
    }

    return sp;
}


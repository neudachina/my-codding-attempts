#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t
#include <utility>
#include <type_traits>

template <typename T>
struct Slug {
    void operator()(T* t) {
        delete t;
    }

    template <class U>
    Slug& operator=(Slug<U>&&) {
        return *this;
    }
};

template <typename T>
struct Slug<T[]> {
    void operator()(T* t) {
        delete[] t;
    }
};

// Primary template
template <typename T, typename Deleter = Slug<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter()) {
    }

    template <class Del>
    UniquePtr(T* ptr, Del&& deleter) : pair_(ptr, std::forward<Del>(deleter)) {
    }

    template <class U, class AnotherDel>
    UniquePtr(UniquePtr<U, AnotherDel>&& other) noexcept {
        pair_.GetFirst() = static_cast<T*>(other.Release());
        pair_.GetSecond() = std::move(other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    // template <class Del>
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (other.pair_.GetFirst() != pair_.GetFirst()) {
            Reset(static_cast<T*>(other.Release()));
            pair_.GetSecond() = std::forward<Deleter>(other.GetDeleter());
        }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        GetDeleter()(Get());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto to_return = Get();
        pair_.GetFirst() = nullptr;
        return to_return;
    }
    void Reset(T* ptr = nullptr) {
        auto tmp = Get();
        pair_.GetFirst() = ptr;
        GetDeleter()(tmp);
    }

    void Swap(UniquePtr& other) {
        // std::swap(Get(), other.Get());
        // std::swap(GetDeleter(), other.GetDeleter());
        std::swap(pair_, other.pair_);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() {
        return pair_.GetFirst();
    }

    const T* Get() const {
        return pair_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    }
    explicit operator bool() const {
        return (Get() != nullptr);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() {
        return *Get();
    }
    T* operator->() {
        return Get();
    }

    const std::add_lvalue_reference_t<T> operator*() const {
        return *Get();
    }
    const T* operator->() const {
        return Get();
    }

private:
    CompressedPair<T*, Deleter> pair_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    // Constructors

    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter()) {
    }

    template <class Del>
    UniquePtr(T* ptr, Del&& deleter) : pair_(ptr, std::forward<Del>(deleter)) {
    }

    template <class U, class AnotherDel>
    UniquePtr(UniquePtr<U, AnotherDel>&& other) noexcept {
        *Get() = static_cast<T*>(other.Release());
        GetDeleter() = std::move(other.pair_.GetSecond());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    // template <class Del>
    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (other.pair_.GetFirst() != pair_.GetFirst()) {
            Reset(static_cast<T*>(other.Release()));
            GetDeleter() = std::forward<Deleter>(other.GetDeleter());
        }
        return *this;
    }
    UniquePtr& operator=(std::nullptr_t) {
        Reset();
        return *this;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        GetDeleter()(Get());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto to_return = Get();
        pair_.GetFirst() = nullptr;
        return to_return;
    }
    void Reset(T* ptr = nullptr) {
        auto tmp = Get();
        pair_.GetFirst() = ptr;
        GetDeleter()(tmp);
    }

    void Swap(UniquePtr& other) {
        std::swap(Get(), other.Get());
        std::swap(GetDeleter(), other.GetDeleter());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_.GetFirst();
    }

    Deleter& GetDeleter() {
        return pair_.GetSecond();
    }
    const Deleter& GetDeleter() const {
        return pair_.GetSecond();
    }
    explicit operator bool() const {
        return (Get() != nullptr);
    }

    T& operator[](size_t idx) {
        return Get()[idx];
    }

    T& operator[](size_t idx) const {
        return Get()[idx];
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    // template <class U>
    const std::add_lvalue_reference_t<T[]> operator*() const {
        return *Get();
    }
    const T* operator->() const {
        return Get();
    }

    std::add_lvalue_reference_t<T[]> operator*() {
        return *Get();
    }
    T* operator->() {
        return Get();
    }

private:
    CompressedPair<T*, Deleter> pair_;
};

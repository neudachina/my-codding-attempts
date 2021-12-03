#pragma once

#include <type_traits>
#include <utility>
#include <cstdint>

template <typename T, uint8_t I, bool = std::is_empty_v<T> && !std::is_final_v<T>>
class CompressedPairElement {
public:
    template <class U>
    explicit CompressedPairElement(U&& value) : value_(std::forward<U>(value)) {
    }

    CompressedPairElement() = default;

    template <class U>
    T& operator=(U&& other) {
        value_ = std::forward<U>(other);
        return value_;
    }

    T& Get() {
        return value_;
    }

    const T& Get() const {
        return value_;
    }

private:
    T value_;
};

template <typename T, uint8_t I>
class CompressedPairElement<T, I, true> : public T {
public:
    CompressedPairElement(T&&) {
    }

    CompressedPairElement() = default;

    template <class U>
    T& operator=(U&&) {
        return *this;
    }

    T& Get() {
        return *this;
    }

    const T& Get() const {
        return *this;
    }
};

template <typename F, typename S>
class CompressedPair : private CompressedPairElement<F, 0>, private CompressedPairElement<S, 1> {
    using First = CompressedPairElement<F, 0>;
    using Second = CompressedPairElement<S, 1>;

public:
    template <typename T, typename U>
    CompressedPair(T&& first, U&& second)
        : First(std::forward<T>(first)), Second(std::forward<U>(second)) {
    }

    CompressedPair() : First(), Second() {
    }

    F& GetFirst() {
        return First::Get();
    };

    S& GetSecond() {
        return Second::Get();
    };

    const F& GetFirst() const {
        return First::Get();
    };

    const S& GetSecond() const {
        return Second::Get();
    };
};

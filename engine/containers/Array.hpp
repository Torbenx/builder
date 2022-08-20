#pragma once

namespace stx {

template<typename T, int_t N>
struct Array {
    using value_type = T;
    using iterator = T*;
    using size_type = int_t;

    T elements[N];
    constexpr T* data() { return elements; }
    constexpr const T* data() const { return elements; }
    constexpr T* begin() { return data(); }
    constexpr const T* begin() const { return data(); }
    constexpr T* end() { return data() + N; }
    constexpr const T* end() const { return data() + N; }
    constexpr int_t size() const { return N; }
    constexpr bool empty() const { return false; }
    constexpr T& operator[](int_t i) { return data()[i]; }
    constexpr const T& operator[](int_t i) const { return data()[i]; }
    constexpr T& front() { return data()[0]; }
    constexpr const T& front() const { return data()[0]; }
    constexpr T& back() { return data()[size() - 1]; }
    constexpr const T& back() const { return data()[size() - 1]; }
};
template<typename T>
struct Array<T, 0> {
    using value_type = T;
    using iterator = T*;
    using size_type = int_t;

    constexpr T* data() { return nullptr; }
    constexpr const T* data() const { return nullptr; }
    constexpr T* begin() { return nullptr; }
    constexpr const T* begin() const { return nullptr; }
    constexpr T* end() { return nullptr; }
    constexpr const T* end() const { return nullptr; }
    constexpr int_t size() const { return 0; }
    constexpr bool empty() const { return true; }
};

}

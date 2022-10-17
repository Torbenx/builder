#pragma once

#include "util/types.hpp"
#include <array>
#include <compare>
#include <concepts>
#include <type_traits>
#include <vector>

namespace Tag {
struct Containers {
    static constexpr char tag[] = "contain";
    static constexpr bool verifyEnabled = true;
};
}

template<typename T>
class ArrayView {
public:
    constexpr ArrayView() { }

    template<int_t N>
    constexpr ArrayView(T (&data)[N])
        : m_data(data), m_size(N) { }

    template<size_t N>
    constexpr ArrayView(std::array<T, N>& data)
        : m_data(data.data()), m_size(N) { }

    template<size_t N>
    constexpr ArrayView(const std::array<std::remove_const_t<T>, N>& data) requires std::is_const_v<T>
        : m_data(data.data()), m_size(N) { }

    constexpr ArrayView(T* data, int_t size)
        : m_data(data), m_size(size) { }

    constexpr ArrayView(std::remove_const_t<T>* data, int_t size) requires std::is_const_v<T>
        : m_data(data), m_size(size) { }

    constexpr ArrayView(std::vector<T>& data)
        : m_data(data.data()), m_size(data.size()) { }

    constexpr ArrayView(const std::vector<std::remove_const_t<T>>& data) requires std::is_const_v<T>
        : m_data(data.data()), m_size(data.size()) { }

    constexpr ArrayView(const std::initializer_list<T>& data)
        : m_data(std::data(data)), m_size(data.size()) { }

    constexpr T* data() const { return m_data; }
    constexpr int_t size() const { return m_size; }
    constexpr T& operator[](int_t i) const { return data()[i]; }
    constexpr T* begin() const { return data(); }
    constexpr T* end() const { return data() + size(); }
    constexpr bool empty() const { return size() == 0; }

private:
    T* m_data = nullptr;
    int_t m_size = 0;
};

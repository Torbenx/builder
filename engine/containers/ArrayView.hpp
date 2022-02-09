#pragma once

#include "util/types.hpp"
#include <array>
#include <vector>
#include <compare>
#include <concepts>
#include <type_traits>

namespace Tag {
struct Containers {
    static constexpr char tag[] = "contain";
    static constexpr bool verifyEnabled = true;
};
}

template<typename T>
class ArrayView {
public:
    ArrayView() { }

    template<int_t N>
    ArrayView(T (&data)[N])
        : m_data(data), m_size(N) { }

    template<size_t N>
    ArrayView(std::array<T, N>& data)
        : m_data(data.data()), m_size(N) { }

    template<size_t N>
    ArrayView(const std::array<std::remove_const_t<T>, N>& data) requires std::is_const_v<T>
        : m_data(data.data()), m_size(N) { }

    ArrayView(T* data, int_t size)
        : m_data(data), m_size(size) { }

    ArrayView(std::remove_const_t<T>* data, int_t size) requires std::is_const_v<T>
        : m_data(data), m_size(size) { }

    ArrayView(std::vector<T>& data)
        : m_data(data.data()), m_size(data.size()) { }

    ArrayView(const std::vector<std::remove_const_t<T>>& data) requires std::is_const_v<T>
        : m_data(data.data()), m_size(data.size()) { }

    ArrayView(const std::initializer_list<T>& data)
        : m_data(std::data(data)), m_size(data.size()) { }

    T* data() const { return m_data; }
    int_t size() const { return m_size; }
    T& operator[](int_t i) const { return data()[i]; }
    T* begin() const { return data(); }
    T* end() const { return data() + size(); }

    void release() {
        delete[] m_data;
        m_data = nullptr;
        m_size = 0;
    }

private:
    T* m_data = nullptr;
    int_t m_size = 0;
};

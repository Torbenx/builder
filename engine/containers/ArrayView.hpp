#pragma once

#include "util/types.hpp"
#include <concepts>
#include <type_traits>

template<typename T>
class ArrayView {
public:
    ArrayView() { }

    template<int_t N>
    ArrayView(T (&data)[N])
        : m_data(data), m_size(N) { }

    ArrayView(T* data, int_t size)
        : m_data(data), m_size(size) { }

    T* data() const { return m_data; }
    int_t size() const { return m_size; }
    T& operator[](int_t i) const { return data()[i]; }

private:
    T* m_data = nullptr;
    int_t m_size = 0;
};

#pragma once

#include "containers/ArrayView.hpp"
#include <vector>
#include <concepts>

template<typename /*std::equality_comparable*/ T>
class FlatSet {
public:
    using Iterator = typename std::vector<T>::iterator;

    Iterator begin() { return m_data.begin(); }
    Iterator end() { return m_data.end(); }
    int_t size() { return m_data.size(); }
    void reserve(int_t n) { m_data.reserve(n); }
    void reserveForInsertions(int_t n) { reserve(size() + n); }
    T& operator[](int_t i) { return m_data[i]; }
    T* data() { return m_data.data(); }
    operator ArrayView<T>() { return { data(), size() }; }

    int_t find(const T& t) {
        for (int_t i = 0; i < size(); i++) {
            if (t == m_data[i]) {
                return i;
            }
        }
        return size();
    }
    bool has(const T& t) { return find(t) != size(); }

    void insert(const T& t) {
        m_data.push_back(t);
    }
    void insert(ArrayView<T> arr) {
        int_t oldSize = size();
        m_data.resize(oldSize + arr.size());
        std::memcpy(data() + oldSize, arr.data(), arr.size() * sizeof(T));
    }

    void removeAt(int_t i) {
        m_data[i] = m_data[size() - 1];
        m_data.pop_back();
    }
    void remove(const T& t) { removeAt(find(t)); }

private:
    std::vector<T> m_data;
};
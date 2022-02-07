#pragma once

#include "containers/ArrayView.hpp"
#include <vector>
#include <concepts>
#include <cstring>

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

    template<typename S>
    int_t find(const S& s) {
        for (int_t i = 0; i < size(); i++) {
            if (s == m_data[i])
                return i;
        }
        return size();
    }
    bool has(const T& t) { return find(t) != size(); }
    template<typename S>
    T& get(const S& s) { return m_data[find<S>(s)]; }

    void insert(const T& t) {
        m_data.push_back(t);
    }
    void insert(ArrayView<T> arr) {
        m_data.insert(end(), arr.begin(), arr.end());
    }

    void removeAt(int_t i) {
        m_data[i] = std::move(m_data[size() - 1]);
        m_data.pop_back();
    }
    void remove(const T& t) { removeAt(find(t)); }

    int_t findOrInsert(const T& t) {
        int_t r = find(t);
        if (r == size())
            insert(t);
        return r;
    }
    T& getOrInsert(const T& t) {
        return m_data[findOrInsert(t)];
    }

    template<class UnaryPredicate>
    void removeIf(UnaryPredicate&& p) {
        for (int_t i = 0; i < size(); i++) {
            if (p(data()[i])) {
                removeAt(i);
                i--;
            }
        }
    }

    void clear() {
        m_data.clear();
    }

private:
    std::vector<T> m_data;
};

#pragma once

#include "math/solver/SparseMatrix.hpp"
#include <iostream>
#include <memory>

template<typename T, typename size_type>
class DenseVector {
public:
    using Vector = DenseVector<T, size_type>;

    DenseVector() { }

    DenseVector(size_type size)
        : data(size) { }

    void add(Vector& other, T factor) {
        size_type size = std::min(size(), other.size());
        for (size_type i = 0; i < size; i++) {
            at(i) += factor * other[i];
        }
    }

    template<class ZeroTester>
    void add(SparseVector<T, size_type, ZeroTester>& other, T factor, size_type offset = 0, size_type end = std::numeric_limits<size_type>::max()) {
        end = std::min(end, other.size());
        for (size_type i = offset; i < end; i++) {
            if (other[i].index < size()) {
                at(other[i].index) += factor * other[i].value;
            } else {
                break;
            }
        }
    }

    size_type size() { return data.size(); }

    T& operator[](size_type index) { return data[index]; }

    void debug() {
        for (size_type i = 0; i < size(); i++) {
            std::cout << at(i) << ", ";
        }
        std::cout << "\n\n";
    }

private:
    std::vector<T> data;

    T& at(size_type index) { return data[index]; }
};

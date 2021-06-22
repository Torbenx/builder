#pragma once

#include "math/solver/SparseVector.hpp"
#include <algorithm>
#include <iostream>
#include <vector>

template<typename T, typename size_type, class ZeroTester>
class SparseMatrix {
public:
    using Matrix = SparseMatrix<T, size_type, ZeroTester>;
    using Column = SparseVector<T, size_type, ZeroTester>;
    using Entry = typename Column::Entry;

    std::vector<Column> columns;

    size_type rowCount() {
        size_type lastIndex = 0;
        for (Column& column : columns) {
            if (!column.empty())
                lastIndex = std::max(lastIndex, column.back().index);
        }
        return lastIndex + 1;
    }

    void setIdentity(size_type size) {
        columns.clear();
        columns.resize(size);
        for (size_type i = 0; i < size; i++) {
            columns[i].pushBack(Entry(1, i));
        }
    }

    void setEmpty(size_type size) {
        columns.clear();
        columns.resize(size);
    }

    void leftMultiplyBy(Matrix& l) {
        size_type columnCount = columns.size();
        Column temp;
        for (size_type i = 0; i < columnCount; i++) {
            temp.clear();
            for (Entry entry : columns[i]) {
                temp.mask |= l.columns[entry.index].mask;
            }
            temp.reserve();
            for (Entry entry : columns[i]) {
                temp.add_internal(l.columns[entry.index], entry.value);
            }
            std::swap(temp, columns[i]);
        }
    }

    void leftMultiplyAccumulateBy(Matrix& l) {
        size_type columnCount = columns.size();
        Column temp;
        for (size_type i = 0; i < columnCount; i++) {
            temp = columns[i];
            for (Entry entry : columns[i]) {
                temp.mask |= l.columns[entry.index].mask;
            }
            temp.reserve();
            for (Entry entry : columns[i]) {
                temp.add_internal(l.columns[entry.index], entry.value);
            }
            std::swap(temp, columns[i]);
        }
    }

    void add(Matrix& other, T factor) {
        size_type columnCount = std::min(columns.size(), other.columns.size());
        for (size_type i = 0; i < columnCount; i++) {
            columns[i].add(other.columns[i], factor);
        }
    }

    T* asMatrix() { return asMatrix(rowCount()); }

    T* asMatrix(size_type rowCount) {
        size_type columnCount = columns.size();
        T* m = new T[rowCount * columnCount];
        memset(m, 0, rowCount * columnCount * sizeof(T));
        for (size_type column = 0; column < columnCount; column++) {
            for (Entry entry : columns[column]) {
                if (entry.index < rowCount)
                    m[column * rowCount + entry.index] = entry.value;
            }
        }
        return m;
    }

    void debug() {
        size_type rowCount = this->rowCount();
        size_type columnCount = columns.size();
        T* mat = asMatrix(rowCount);
        for (size_type i = 0; i < rowCount; i++) {
            std::cout << "{" << mat[i];
            for (size_type j = 1; j < columnCount; j++) {
                std::cout << ", " << mat[i + j * rowCount];
            }
            std::cout << "},\n";
        }
        std::cout << "\n";
        delete mat;
    }
};

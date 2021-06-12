#pragma once

#include "math/solver/DenseVector.hpp"
#include "math/solver/SparseMatrix.hpp"

struct SparseOps {

    template<typename T, typename size_type, class ZeroTester>
    static DenseVector<T, size_type> multiply(SparseMatrix<T, size_type, ZeroTester>& matrix, DenseVector<T, size_type>& vector) {
        DenseVector<T, size_type> out(vector.size());
        size_type size = std::min(vector.size(), (size_type)matrix.columns.size());
        for (size_type i = 0; i < size; i++) {
            out.add(matrix.columns[i], vector[i]);
        }
        return out;
    }

    template<typename T, typename size_type, class ZeroTester>
    static DenseVector<T, size_type> multiply(DenseVector<T, size_type>& vector, SparseMatrix<T, size_type, ZeroTester>& matrix) {
        DenseVector<T, size_type> out(vector.size());
        size_type size = std::min(vector.size(), (size_type)matrix.columns.size());
        for (size_type i = 0; i < size; i++) {
            out[i] = SparseOps::dot(vector, matrix.columns[i]);
        }
        return out;
    }

    template<typename T, typename size_type, class ZeroTester>
    static T dot(DenseVector<T, size_type>& v1, SparseVector<T, size_type, ZeroTester>& v2, size_type offset = 0, size_type end = std::numeric_limits<size_type>::max()) {
        end = std::min(end, v2.size());
        T sum = 0;
        for (size_type i = offset; i < end; i++) {
            if (v2[i].index >= v1.size())
                break;
            sum += v1[v2[i].index] * v2[i].value;
        }
        return sum;
    }

    template<typename T, typename size_type, class ZeroTester>
    static SparseMatrix<T, size_type, ZeroTester> multiply(SparseMatrix<T, size_type, ZeroTester>& l, SparseMatrix<T, size_type, ZeroTester>& r) {
        size_type columnCount = r.columns.size();
        SparseMatrix<T, size_type, ZeroTester> out;
        out.setEmpty(columnCount);

        for (size_type i = 0; i < columnCount; i++) {
            uint64_t mask = 0;
            for (auto entry : r.columns[i]) {
                mask |= l.columns[entry.index].mask;
            }
            out.columns[i].reserveForAdd(mask);
            out.columns[i].mask = mask;
            for (auto entry : r.columns[i]) {
                out.columns[i].add_internal(l.columns[entry.index], entry.value);
            }
        }

        return out;
    }
};

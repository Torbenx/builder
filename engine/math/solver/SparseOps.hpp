#pragma once

#include "math/solver/DenseVector.hpp"
#include "math/solver/SparseMatrix.hpp"

struct SparseOps {

    template<typename T, typename size_type, class ZeroTester>
    static void multiply(DenseVector<T, size_type>& out, SparseMatrix<T, size_type, ZeroTester>& matrix, DenseVector<T, size_type>& vector) {
        size_type size = std::min(vector.size(), (size_type)matrix.columns.size());
        for (size_type i = 0; i < size; i++) {
            out.add(matrix.columns[i], vector[i]);
        }
    }

    template<typename T, typename size_type, class ZeroTester>
    static void multiply(DenseVector<T, size_type>& out, DenseVector<T, size_type>& vector, SparseMatrix<T, size_type, ZeroTester>& matrix) {
        size_type size = std::min(out.size(), (size_type)matrix.columns.size());
        for (size_type i = 0; i < size; i++) {
            out[i] = SparseOps::dot(vector, matrix.columns[i]);
        }
    }

    template<typename T, typename size_type, class ZeroTester>
    static T dot(SparseVector<T, size_type, ZeroTester>& v1, SparseVector<T, size_type, ZeroTester>& v2, size_type offset1 = 0, size_type offset2 = 0, size_type end1 = std::numeric_limits<size_type>::max(), size_type end2 = std::numeric_limits<size_type>::max()) {
        end1 = std::min(end1, v1.size());
        end2 = std::min(end2, v2.size());
        if (offset1 >= end1 || offset2 >= end2 || !BitSet::mightShareAny(v1.mask, v2.mask))
            return 0;
        T sum = 0;
        while (true) {
            if (v1[offset1].index == v2[offset2].index) {
                sum += v1[offset1].value * v2[offset2].value;
                offset1++;
                offset2++;
                if (offset1 >= end1 || offset2 >= end2) {
                    break;
                }
            } else if (v1[offset1].index > v2[offset2].index) {
                offset2++;
                if (offset2 >= end2) {
                    break;
                }
            } else {
                offset1++;
                if (offset1 >= end1) {
                    break;
                }
            }
        }
        return sum;
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

    template<typename T, typename size_type>
    static T dot(DenseVector<T, size_type>& v1, DenseVector<T, size_type>& v2) {
        size_type end = std::min(v1.size(), v2.size());
        T sum = 0;
        for (size_type i = 0; i < end; i++) {
            sum += v1[i] * v2[i];
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

    template<typename T, typename size_type, class ZeroTester>
    static void multiply(SparseVector<T, size_type, ZeroTester>& out, SparseVector<T, size_type, ZeroTester>& l, SparseMatrix<T, size_type, ZeroTester>& r) {
        ZeroTester tester;
        out.clear();
        for (size_type i = 0; i < r.columns.size(); i++) {
            T dot = SparseOps::dot(l, r.columns[i]);
            if (!tester(dot)) {
                out.pushBack({ dot, i });
            }
        }
    }

    template<typename T, typename size_type, class ZeroTester>
    static void multiplyAccumulate(DenseVector<T, size_type>& out, SparseVector<T, size_type, ZeroTester>& l, SparseMatrix<T, size_type, ZeroTester>& r) {
        ZeroTester tester;
        for (size_type i = 0; i < r.columns.size(); i++) {
            out[i] += SparseOps::dot(l, r.columns[i]);
        }
    }
};

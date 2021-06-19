#pragma once

#include "math/solver/Permutation.hpp"
#include "math/solver/SparseOps.hpp"
#include "util/LogTimer.hpp"

template<typename T, typename size_type, class ZeroTester>
class LUPDecomposition {
public:
    using Matrix = SparseMatrix<T, size_type, ZeroTester>;
    Matrix l, u;
    Permutation<size_type> p;
    bool isInvertible = false;

    void inverseL(typename Matrix::Column& col) {
        for (size_type i = 0; i < col.size(); i++) {
            // col[i] -= col[row] * l[row,i] ; i > row
            col.add(l.columns[col[i].index], -col[i].value);
        }
    }

    void inverseL(DenseVector<T, size_type>& vector) {
        for (size_type i = 0; i < vector.size(); i++) {
            vector.add(l.columns[i], -vector[i]);
        }
    }

    void inverseU(DenseVector<T, size_type>& vector) {
        size_type size = std::min((size_type)u.columns.size(), vector.size());
        for (size_type i = size - 1; i != -1; i--) {
            vector[i] /= u.columns[i].back().value;
            vector.add(u.columns[i], -vector[i], 0, u.columns[i].size() - 1);
        }
    }

    void inverseU(SparseVector<T, size_type, ZeroTester>& vector) {
        for (size_type i = vector.size() - 1; i != -1; i--) {
            vector[i].value /= u.columns[vector[i].index].back().value;
            size_type oldSize = vector.size();
            vector.add(u.columns[vector[i].index], -vector[i].value, 0, 0, u.columns[vector[i].index].size() - 1);
            i += vector.size() - oldSize;
        }
    }

    void inverseTransposeL(DenseVector<T, size_type>& vector) {
        size_type size = std::min((size_type)l.columns.size(), vector.size());
        for (size_type i = size - 1; i != -1; i--) {
            vector[i] -= SparseOps::dot(vector, l.columns[i]);
        }
    }

    void inverseTransposeU(DenseVector<T, size_type>& vector) {
        for (size_type i = 0; i < vector.size(); i++) {
            vector[i] -= SparseOps::dot(vector, u.columns[i], (size_type)0, u.columns[i].size() - 1);
            vector[i] /= u.columns[i].back().value;
        }
    }

    void inverse(DenseVector<T, size_type>& vector) {
        inverseL(vector);
        inverseU(vector);
        DenseVector<T, size_type> temp(vector.size());
        for (size_type i = 0; i < vector.size(); i++) {
            temp[p.inverse(i)] = vector[i];
        }
        std::swap(temp, vector);
    }

    void inverseUnsorted(SparseVector<T, size_type, ZeroTester>& vector) {
        inverseL(vector);
        inverseU(vector);
        for (auto& entry : vector) {
            entry.index = p.inverse(entry.index);
        }
    }

    void inverseTranspose(DenseVector<T, size_type>& out, DenseVector<T, size_type>& in) {
        for (size_type i = 0; i < in.size(); i++) {
            out[p[i]] = in[i];
        }
        inverseTransposeU(out);
        inverseTransposeL(out);
    }

    void replaceColumn(size_type column) {
        inverseL(u.columns[column]);
        Matrix oldL(std::move(l));
        l.setEmpty(u.columns.size());
        decompose(column);
        l.leftMultiplyAccumulateBy(oldL);
        l.add(oldL, 1);
    }

    // u holds the matrix to decompose
    void decompose() {
        size_type columnCount = u.columns.size();
        p.setIdentity(columnCount);
        l.setEmpty(columnCount);
        isInvertible = true;
        decompose(0);
    }

private:
    void decompose(size_type startRow) {
        ZeroTester tester;
        size_type columnCount = u.columns.size();
        std::vector<size_type> columnOffsets;
        columnOffsets.resize(columnCount);
        for (size_type i = startRow; i < columnCount; i++) {
            while (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index < startRow) {
                columnOffsets[i]++;
            }
        }

        for (size_type row = startRow; row < columnCount; row++) {
            // select pivot
            size_type pivot = columnCount;
            size_type bestScore = std::numeric_limits<size_type>::max();
            for (size_type i = row; i < columnCount; i++) {
                if (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index == row) {
                    size_type entryCount = u.columns[i].size() - columnOffsets[i] - 1;
                    size_type score = 0;
                    for (typename Matrix::Entry entry : u.columns[i]) {
                        score += columnCount - entry.index;
                    }
                    if (score <= bestScore) {
                        bestScore = score;
                        pivot = i;
                    }
                }
            }
            /*T max = 0;
            for (size_type i = row; i < columnCount; i++) {
                if (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index == row) {
                    if (std::abs(u.columns[i][columnOffsets[i]].value) > max) {
                        pivot = i;
                        max = std::abs(u.columns[i][columnOffsets[i]].value);
                    }
                }
            }*/
            if (pivot == columnCount) {
                isInvertible = false;
                // LOGD("not invertible:");
                // u.debug();
                break;
            }
            if (pivot != row) {
                p.swap(pivot, row);
                std::swap(u.columns[pivot], u.columns[row]);
                std::swap(columnOffsets[pivot], columnOffsets[row]);
            }

            auto& pivotCol = u.columns[row];
            size_type diagOffset = columnOffsets[row];
            T invDiag = 1 / pivotCol[diagOffset].value;

            // l[row,i] = u[column,i] / u[column,row] ; i > row
            size_type entryCount = pivotCol.size() - (diagOffset + 1);
            typename Matrix::Column& pivotFactors = l.columns[row];
            pivotFactors.reserve(entryCount);
            for (size_type i = 0; i < entryCount; i++) {
                typename Matrix::Entry entry = pivotCol[i + diagOffset + 1];
                entry.value *= invDiag;
                pivotFactors.pushBack(entry);
            }
            // u[column,i] = 0 ; i > row
            pivotCol.eraseAfter(diagOffset);

            // u[i,j] -= u[i,row]*u[column,i]/u[column,row] ; i > column, j > row
            for (size_type i = row + 1; i < columnCount; i++) {
                auto& col = u.columns[i];
                size_type offset = columnOffsets[i];

                if (offset == col.size() || col[offset].index > row) {
                    // fast path, do nothing
                    continue;
                }
                col.add(pivotFactors, -col[offset].value, offset);
                columnOffsets[i]++;
            }
        }
    }
};

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
    std::vector<bool> dirtyColumns;

    void inverseL(SparseVector<T, size_type, ZeroTester>& vector) {
        for (size_type i = 0; i < vector.size(); i++) {
            // col[i] -= col[row] * l[row,i] ; i > row
            vector.add(l.columns[vector[i].index], -vector[i].value);
        }
    }

    void inverseL(DenseVector<T, size_type>& vector) {
        for (size_type i = 0; i < vector.size(); i++) {
            vector.add(l.columns[i], -vector[i]);
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

    void inverseU(DenseVector<T, size_type>& vector) {
        size_type size = std::min((size_type)u.columns.size(), vector.size());
        for (size_type i = size - 1; i != -1; i--) {
            vector[i] /= u.columns[i].back().value;
            vector.add(u.columns[i], -vector[i], 0, u.columns[i].size() - 1);
        }
    }

    void inverseTransposeL(SparseVector<T, size_type, ZeroTester>& vector) {
        ZeroTester tester;
        size_type offset = vector.size() - 1;
        for (size_type i = l.columns.size() - 1; i != -1; i--) {
            T dot = SparseOps::dot(l.columns[i], vector);
            if (offset != -1 && vector[offset].index == i) {
                vector[offset].value -= dot;
                offset--;
            } else if (!tester(dot)) {
                vector.insert(offset + 1, { -dot, i });
            }
        }
    }

    void inverseTransposeL(DenseVector<T, size_type>& vector) {
        size_type size = std::min((size_type)l.columns.size(), vector.size());
        for (size_type i = size - 1; i != -1; i--) {
            vector[i] -= SparseOps::dot(vector, l.columns[i]);
        }
    }

    void inverseTransposeU(SparseVector<T, size_type, ZeroTester>& vector) {
        ZeroTester tester;
        size_type offset = 0;
        for (size_type i = 0; i < u.columns.size(); i++) {
            T dot = SparseOps::dot(u.columns[i], vector, (size_type)0, (size_type)0, u.columns[i].size() - 1);
            if (offset < vector.size() && vector[offset].index == i) {
                vector[offset].value -= dot;
                vector[offset].value /= u.columns[i].back().value;
                offset++;
            } else if (!tester(dot)) {
                vector.insert(offset, { -dot / u.columns[i].back().value, i });
                offset++;
            }
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

    void inverse(SparseVector<T, size_type, ZeroTester>& vector) {
        inverseL(vector);
        inverse_internal(vector);
    }

    void inverse_internal(SparseVector<T, size_type, ZeroTester>& vector) {
        inverseU(vector);
        for (auto& entry : vector) {
            entry.index = p.inverse(entry.index);
        }
        vector.sort();
    }

    void inverseTranspose(DenseVector<T, size_type>& out, DenseVector<T, size_type>& in) {
        for (size_type i = 0; i < in.size(); i++) {
            out[p[i]] = in[i];
        }
        inverseTransposeU(out);
        inverseTransposeL(out);
    }

    void inverseTranspose(SparseVector<T, size_type, ZeroTester>& vector) {
        for (auto& entry : vector) {
            entry.index = p[entry.index];
        }
        vector.sort();
        inverseTransposeU(vector);
        inverseTransposeL(vector);
    }

    void replaceColumn(size_type column) {
        inverseL(u.columns[column]);
        replaceColumn_internal(column);
    }

    void replaceColumn_internal(size_type column) {
        dirtyColumns[column] = true;
        decompose(column, [&](size_type row, typename Matrix::Column& pivotCol, size_type offset, T factor) {
            size_type entryCount = pivotCol.size() - offset;
            for (size_type i = 0; i < entryCount; i++) {
                typename Matrix::Entry entry = pivotCol[offset + i];
                l.columns[row].add(l.columns[entry.index], entry.value * factor);
            }
            l.columns[row].add(pivotCol, factor, 0, offset);
        });
    }

    // u holds the matrix to decompose
    void decompose() {
        size_type columnCount = u.columns.size();
        p.setIdentity(columnCount);
        l.setEmpty(columnCount);
        isInvertible = true;
        dirtyColumns.clear();
        dirtyColumns.resize(columnCount);
        dirtyColumns.flip();
        decompose(0, [&](size_type row, typename Matrix::Column& pivotCol, size_type offset, T factor) {
            // l[row,i] = u[column,i] / u[column,row] ; i > row
            size_type entryCount = pivotCol.size() - offset;
            typename Matrix::Column& pivotFactors = l.columns[row];
            pivotFactors.reserve(entryCount);
            for (size_type i = 0; i < entryCount; i++) {
                typename Matrix::Entry entry = pivotCol[offset + i];
                entry.value *= factor;
                pivotFactors.pushBack(entry);
            }
        });
    }

private:
    template<typename F>
    void decompose(size_type startRow, F lambda) {
        ZeroTester tester;
        size_type columnCount = u.columns.size();
        std::vector<size_type> columnOffsets;
        columnOffsets.resize(columnCount);

        for (size_type row = startRow; row < columnCount; row++) {
            if (!dirtyColumns[row]) {
                continue;
            }

            // select pivot
            size_type pivot = columnCount;
            size_type bestScore = std::numeric_limits<size_type>::max();
            for (size_type i = row; i < columnCount; i++) {
                while (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index < row) {
                    columnOffsets[i]++;
                }
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
                LOGD("not invertible!");
                u.debug();
                break;
            }
            if (pivot != row) {
                p.swap(pivot, row);
                std::swap(u.columns[pivot], u.columns[row]);
                std::swap(columnOffsets[pivot], columnOffsets[row]);
                dirtyColumns[pivot] = true;
            }
            dirtyColumns[row] = false;

            size_type diagOffset = columnOffsets[row];
            auto& pivotCol = u.columns[row];
            T invDiag = 1 / pivotCol[diagOffset].value;

            lambda(row, pivotCol, diagOffset + 1, invDiag);

            // u[i,j] -= u[i,row]*u[column,i]/u[column,row] ; i > column, j > row
            for (size_type i = row + 1; i < columnCount; i++) {
                auto& col = u.columns[i];
                size_type offset = columnOffsets[i];

                if (offset == col.size() || col[offset].index > row) {
                    continue;
                }
                if (diagOffset + 1 < pivotCol.size()) {
                    col.add(pivotCol, -col[offset].value * invDiag, 0, diagOffset + 1);
                    dirtyColumns[i] = true;
                }
                columnOffsets[i]++;
            }

            // u[column,i] = 0 ; i > row
            pivotCol.eraseAfter(diagOffset);
        }
    }
};

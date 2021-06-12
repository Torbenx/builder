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
    std::vector<size_type> zeroRows;
    size_type rowCount = 0;

    void inverseL(typename Matrix::Column& col) {
        for (size_type i = 0; i < col.size(); i++) {
            // col[i] -= col[row] * l[row,i] ; i > row
            col.add(l.columns[col[i].index], -col[i].value, i + 1);
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

    void inverseTranspose(DenseVector<T, size_type>& vector) {
        DenseVector<T, size_type> temp(vector.size());
        for (size_type i = 0; i < vector.size(); i++) {
            temp[p[i]] = vector[i];
        }
        std::swap(temp, vector);
        inverseTransposeU(vector);
        inverseTransposeL(vector);
    }

    void replaceColumn(size_type column, typename Matrix::Column&& col) {
        column = p[column];

        if (!col.empty()) {
            size_type lastIndex = col.back().index;
            // update rowCount
            if (lastIndex >= rowCount) {
                rowCount = lastIndex + 1;
                l.columns.resize(rowCount);
            }
            inverseL(col);
        }

        size_type decomposeRow1 = rowCount;
        size_type decomposeColumn1;
        typename std::vector<size_type>::iterator it1;
        for (size_type i = 0, j = 0; i < col.size() && j < zeroRows.size();) {
            if (zeroRows[j] < col[i].index) {
                j++;
            } else if (zeroRows[j] > col[i].index) {
                i++;
            } else {
                decomposeRow1 = zeroRows[j];
                decomposeColumn1 = decomposeRow1 - j;
                it1 = zeroRows.begin() + j;
                break;
            }
        }

        size_type decomposeRow2 = rowCount;
        size_type decomposeColumn2;
        typename std::vector<size_type>::iterator it2;
        if (!u.columns[column].empty()) {
            decomposeRow2 = u.columns[column].back().index;
            decomposeColumn2 = column;
            it2 = std::lower_bound(zeroRows.begin(), zeroRows.end(), decomposeRow2);
            if (decomposeRow2 - decomposeColumn2 != std::distance(zeroRows.begin(), it2)) {
                decomposeRow2 = rowCount;
            }
        }

        std::swap(col, u.columns[column]);

        if (decomposeRow2 < decomposeRow1) {
            decomposeRow1 = decomposeRow2;
            decomposeColumn1 = decomposeColumn2;
            it1 = it2;
        }

        if (decomposeRow1 == rowCount) {
            return;
        }

        zeroRows.erase(it1, zeroRows.end());
        Matrix oldL(std::move(l));
        l.setEmpty(rowCount);
        decompose(decomposeRow1, decomposeColumn1);
        l.leftMultiplyAccumulateBy(oldL);
        l.add(oldL, 1);

        col.clear();
    }

    void decompose(Matrix&& m) {
        u.columns.clear();
        std::swap(u, m);

        rowCount = u.rowCount();
        p.setIdentity(u.columns.size());
        zeroRows.clear();
        l.setEmpty(rowCount);
        decompose(0, 0);
    }

private:
    void decompose(size_type startRow, size_type startColumn) {
        ZeroTester tester;
        size_type columnCount = u.columns.size();
        std::vector<size_type> columnOffsets;
        columnOffsets.resize(columnCount);
        for (size_type i = startColumn; i < columnCount; i++) {
            while (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index < startRow) {
                columnOffsets[i]++;
            }
        }

        size_type row = startRow;
        size_type column = startColumn;
        while (row < rowCount && column < columnCount) {
            // select pivot
            size_type pivot = columnCount;
            size_type bestScore = std::numeric_limits<size_type>::max();
            for (size_type i = column; i < columnCount; i++) {
                if (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index == row) {
                    size_type entryCount = u.columns[i].size() - columnOffsets[i] - 1;
                    size_type score = 0;
                    for (typename Matrix::Entry entry : u.columns[i]) {
                        score += rowCount - entry.index;
                    }
                    if (score <= bestScore) {
                        bestScore = score;
                        pivot = i;
                    }
                }
            }
            /*T max = 0;
            for (size_type i = column; i < columnCount; i++) {
                if (columnOffsets[i] < u.columns[i].size() && u.columns[i][columnOffsets[i]].index == row) {
                    if (std::abs(u.columns[i][columnOffsets[i]].value) > max) {
                        pivot = i;
                        max = std::abs(u.columns[i][columnOffsets[i]].value);
                    }
                }
            }*/
            if (pivot == columnCount) {
                // row - column = # of entries in zeroRows less then row
                zeroRows.push_back(row);
                row++;
                continue;
            }
            if (pivot != column) {
                p.swap(pivot, column);
                std::swap(u.columns[pivot], u.columns[column]);
                std::swap(columnOffsets[pivot], columnOffsets[column]);
            }

            auto& pivotCol = u.columns[column];
            size_type diagOffset = columnOffsets[column];
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
            pivotCol.eraseAfter(columnOffsets[column]);

            // u[i,j] -= u[i,row]*u[column,i]/u[column,row] ; i > column, j > row
            for (size_type i = column + 1; i < columnCount; i++) {
                auto& col = u.columns[i];
                size_type offset = columnOffsets[i];

                if (offset == col.size() || col[offset].index > row) {
                    // fast path, do nothing
                    continue;
                }
                col.add(pivotFactors, -col[offset].value * invDiag, offset);
                columnOffsets[i]++;
            }

            row++;
            column++;
        }
        rowCount = row;
        l.columns.resize(rowCount);
    }
};

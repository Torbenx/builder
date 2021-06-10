#pragma once

#include "math/SparseMatrix.hpp"
#include "util/LogTimer.hpp"

template<typename size_type>
class Permutation {
public:
    size_type& operator[](size_type in) {
        return inv[in];
    }

    void setIdentity(size_type size) {
        map.resize(size);
        inv.resize(size);
        for (size_type i = 0; i < size; i++) {
            map[i] = i;
            inv[i] = i;
        }
    }

    void swap(size_type in1, size_type in2) {
        //inv[map[in1]] => in1;
        //inv[map[in2]] => in2;

        std::swap(map[in1], map[in2]);

        //inv[map[in1]] => in2;
        //inv[map[in2]] => in1;

        inv[map[in1]] = in1;
        inv[map[in2]] = in2;
    }

    size_type size() {
        return map.size();
    }

private:
    std::vector<size_type> map, inv;
};

template<typename T, typename size_type, class ZeroTester>
class LUPDecomposition {
public:
    using Matrix = SparseMatrix<T, size_type, ZeroTester>;
    Matrix l, u;
    Permutation<size_type> p;
    std::vector<size_type> zeroRows;
    size_type rowCount = 0;

    void inverseL(typename Matrix::Column& col) {
        for (size_type i = 0; i < col.entries.size(); i++) {
            // col[i] -= col[row] * l[row,i] ; i > row
            col.add(l.columns[col.entries[i].index], -col.entries[i].value, i + 1);
        }
    }

    void replaceColumn(size_type column, typename Matrix::Column&& col) {
        //LogTimer timer;
        column = p[column];

        // update rowCount
        if (!col.entries.empty()) {
            size_type lastIndex = col.entries.back().index;
            if (lastIndex >= rowCount) {
                rowCount = lastIndex + 1;
                l.columns.resize(rowCount);
            }
        }
        inverseL(col);
        //timer();

        std::swap(col, u.columns[column]);
        if (!col.entries.empty()) {
            size_type oldLastIndex = col.entries.back().index;
            auto it = std::lower_bound(zeroRows.begin(), zeroRows.end(), oldLastIndex);
            if (oldLastIndex - column == std::distance(zeroRows.begin(), it)) {
                zeroRows.erase(it, zeroRows.end());
                Matrix oldL(std::move(l));
                l.setEmpty(rowCount);
                //timer();
                decompose(oldLastIndex, column);
                //timer();
                Matrix temp = Matrix::multiply(oldL, l);
                temp.add(oldL, 1);
                temp.add(l, 1);
                l = std::move(temp);
            }
        }
        col.entries.clear();
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
            while (columnOffsets[i] < u.columns[i].entries.size() && u.columns[i].entries[columnOffsets[i]].index < startRow) {
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
                if (columnOffsets[i] < u.columns[i].entries.size() && u.columns[i].entries[columnOffsets[i]].index == row) {
                    size_type entryCount = u.columns[i].entries.size() - columnOffsets[i] - 1;
                    size_type score = 0;
                    for (typename Matrix::Entry entry : u.columns[i].entries) {
                        score += rowCount - entry.index;
                    }
                    if (score <= bestScore) {
                        bestScore = score;
                        pivot = i;
                    }
                }
            }
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
            T invDiag = 1 / pivotCol.entries[diagOffset].value;

            // l[row,i] = u[column,i] / u[column,row] ; i > row
            size_type entryCount = pivotCol.entries.size() - (diagOffset + 1);
            typename Matrix::Column& pivotFactors = l.columns[row];
            pivotFactors.entries.reserve(entryCount);
            for (size_type i = 0; i < entryCount; i++) {
                typename Matrix::Entry entry = pivotCol.entries[i + diagOffset + 1];
                entry.value *= invDiag;
                pivotFactors.entries.push_back(entry);
            }
            // u[column,i] = 0 ; i > row
            pivotCol.entries.erase(pivotCol.entries.begin() + columnOffsets[column] + 1, pivotCol.entries.end());

            // u[i,j] -= u[i,row]*u[column,i]/u[column,row] ; i > column, j > row
            for (size_type i = column + 1; i < columnCount; i++) {
                auto& col = u.columns[i];
                size_type offset = columnOffsets[i];

                if (offset == col.entries.size() || col.entries[offset].index > row) {
                    // fast path, do nothing
                    continue;
                }
                T top = col.entries[offset].value;
                col.add(pivotFactors, -col.entries[offset].value*invDiag, offset);
                columnOffsets[i]++;
            }

            row++;
            column++;
        }
        rowCount = row;
        l.columns.resize(rowCount);
    }
};

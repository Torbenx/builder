#pragma once

#include "math/SparseMatrix.hpp"

template<typename size_type>
class Permutation {
public:
    std::vector<size_type> map;

    size_type& operator[](size_type in) {
        return map[in];
    }

    void setIdentity(size_type size) {
        map.resize(size);
        for (size_type i = 0; i < size; i++) {
            map[i] = i;
        }
    }

    void swap(size_type in1, size_type in2) {
        size_type temp = map[in1];
        map[in1] = map[in2];
        map[in2] = temp;
    }

    size_type size() {
        return map.size();
    }
};

template<typename T, typename size_type, class ZeroTester>
class LUPDecomposition {
public:
    using Matrix = SparseMatrix<T, size_type, ZeroTester>;
    Matrix l, u;
    Permutation<size_type> p;

    void decompose(Matrix&& m) {
        std::swap(u, m);
        ZeroTester tester;

        size_type columnCount = u.columns.size();
        size_type rowCount = u.rowCount();
        std::vector<size_type> columnOffsets;
        columnOffsets.resize(columnCount);
        p.setIdentity(columnCount);
        l.columns.clear();
        l.setIdentity(rowCount);

        size_type column = 0;
        for (size_type row = 0; row < rowCount; row++) {
            // select pivot
            size_type pivot = column;
            while (pivot < columnCount && (columnOffsets[pivot] == u.columns[pivot].entries.size() || u.columns[pivot].entries[columnOffsets[pivot]].index > row)) {
                pivot++;
            }
            if (pivot == columnCount) {
                // null row
                continue;
            }
            if (pivot != column) {
                std::swap(p[pivot], p[column]);
                std::swap(u.columns[pivot], u.columns[column]);
                std::swap(columnOffsets[pivot], columnOffsets[column]);
            }

            auto& pivotCol = u.columns[column];
            size_type diagOffset = columnOffsets[column];
            T invDiag = 1 / pivotCol.entries[diagOffset].value;

            // l[row,i] = u[column,i] / u[column,row] ; i > row
            size_type entryCount = pivotCol.entries.size() - (diagOffset + 1);
            l.columns[row].entries.reserve(entryCount);
            for(size_type i=0;i<entryCount;i++) {
                typename Matrix::Entry entry = pivotCol.entries[i+diagOffset+1];
                entry.value *= invDiag;
                l.columns[row].entries.push_back(entry);
            }

            // u[column,i] = u[column,i] - u[column,row]*u[column,i]/u[column,row] ; i > row
            for (size_type i = column + 1; i < columnCount; i++) {
                auto& col = u.columns[i];
                size_type offset = columnOffsets[i];

                if (offset == col.entries.size() || col.entries[offset].index > row) {
                    // fast path, do nothing
                    continue;
                }
                T top = col.entries[offset].value;
                size_type pivotOffset = columnOffsets[column] + 1;
                while (offset < col.entries.size() || pivotOffset < pivotCol.entries.size()) {
                    size_type index = offset < col.entries.size() ? col.entries[offset].index : rowCount;
                    size_type pivotIndex = pivotOffset < pivotCol.entries.size() ? pivotCol.entries[pivotOffset].index : rowCount;
                    if (index == pivotIndex) {
                        col.entries[offset].value = col.entries[offset].value - pivotCol.entries[pivotOffset].value * top * invDiag;
                        if (tester(col.entries[offset].value)) {
                            col.entries.erase(col.entries.begin() + offset);
                            offset--;
                        }
                        offset++;
                        pivotOffset++;
                    } else if (index < pivotIndex) {
                        offset++;
                    } else {
                        col.entries.insert(col.entries.begin() + offset, typename Matrix::Entry(-pivotCol.entries[pivotOffset].value * top * invDiag, pivotIndex));
                        offset++;
                        pivotOffset++;
                    }
                }

                columnOffsets[i]++;
            }

            // u[column,i] = 0 ; i > row
            pivotCol.entries.erase(pivotCol.entries.begin() + columnOffsets[column] + 1, pivotCol.entries.end());
            column++;
        }
    }

private:
};

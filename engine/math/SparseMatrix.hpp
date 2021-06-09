#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

template<typename T, typename size_type, class ZeroTester>
class SparseMatrix {
public:
    using Matrix = SparseMatrix<T, size_type, ZeroTester>;

    struct Entry {
        Entry(T value, size_type index)
            : value(value), index(index) { }
        T value;
        size_type index;

        bool operator<(Entry other) {
            return index < other.index;
        }
        bool operator<(size_type index) {
            return this->index < index;
        }
    };

    class Column {
    public:
        using iterator = typename std::vector<Entry>::iterator;
        std::vector<Entry> entries;

        iterator end() { return entries.end(); }
        Entry& back() { return entries.back(); }

        iterator addTo(size_type index, T value) {
            iterator it = getAndConstructEntry(index);
            it->value += value;
            return it;
        }

        iterator getEntry(size_type index) {
            iterator it = find(index);
            return (it == end() || it->index != index) ? end() : it;
        }

        iterator getAndConstructEntry(size_type index) {
            iterator it = find(index);
            if (it == end() || it->index != index) {
                return entries.insert(it, { 0, index });
            }
            return it;
        }

        iterator find(size_type index) {
            return std::lower_bound(entries.begin(), entries.end(), index);
        }
    };

    std::vector<Column> columns;

    size_type rowCount() {
        size_type lastIndex = 0;
        for (Column& column : columns) {
            if (!column.entries.empty())
                lastIndex = std::max(lastIndex, column.back().index);
        }
        return lastIndex + 1;
    }

    void setIdentity(size_type size) {
        columns.clear();
        columns.resize(size);
        for (size_type i = 0; i < size; i++) {
            columns[i].entries.push_back(Entry(1, i));
        }
    }

    void setEmpty(size_type size) {
        columns.clear();
        columns.resize(size);
    }

    static Matrix multiply(Matrix& l, Matrix& r) {
        ZeroTester tester;
        size_type lColumnCount = l.columns.size();
        std::vector<typename Column::iterator> columnIters;
        columnIters.reserve(lColumnCount);
        uint64_t mask = 0;
        for (size_type i = 0; i < lColumnCount; i++) {
            columnIters.push_back(l.columns[i].entries.begin());
            if ((!l.columns[i].entries.empty()) && l.columns[i].entries[0].index == 0)
                mask |= 1 << i;
        }
        size_type lRowCount = l.rowCount();
        size_type rColumnCount = r.columns.size();
        Matrix out;
        out.setEmpty(rColumnCount);

        for (size_type row = 0; row < lRowCount; row++) {
            for (size_type column = 0; column < rColumnCount; column++) {
                T sum = 0;
                for (Entry entry : r.columns[column].entries) {
                    if (entry.index >= lColumnCount)
                        break;
                    if (mask & (1 << entry.index))
                        sum += entry.value * columnIters[entry.index]->value;
                }
                if (!tester(sum)) {
                    out.columns[column].entries.push_back(Entry(sum, row));
                }
            }
            mask = 0;
            for (size_type i = 0; i < lColumnCount; i++) {
                auto& it = columnIters[i];
                if (it != l.columns[i].entries.end()) {
                    if (it->index == row)
                        if (++it == l.columns[i].entries.end())
                            continue;
                    if (it->index == row + 1)
                        mask |= 1 << i;
                }
            }
        }
        return out;
    }

    T* asMatrix() { return asMatrix(rowCount()); }

    T* asMatrix(size_type rowCount) {
        size_type columnCount = columns.size();
        T* m = new T[rowCount * columnCount];
        memset(m, 0, rowCount * columnCount * sizeof(T));
        for (size_type column = 0; column < columnCount; column++) {
            for (auto entry : columns[column].entries) {
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
            for (size_type j = 0; j < columnCount; j++) {
                std::cout << mat[i + j * rowCount] << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
        delete mat;
    }
};

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
            ZeroTester tester;
            if (tester(value))
                return getEntry(index);
            iterator it = getAndConstructEntry(index);
            it->value += value;
            if (tester(it->value)) {
                entries.erase(it);
                return end();
            }
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

        void add(Column& other, T factor, size_type offset = 0, size_type otherOffset = 0) {
            ZeroTester tester;
            for (; otherOffset < other.entries.size(); otherOffset++) {
                auto entry = other.entries[otherOffset];
                entry.value *= factor;
                while (offset < entries.size() && entries[offset].index < entry.index) {
                    offset++;
                }
                if (offset >= entries.size()) {
                    break;
                }
                if (entries[offset].index == entry.index) {
                    entries[offset].value += entry.value;
                    if (tester(entries[offset].value)) {
                        entries.erase(entries.begin() + offset);
                    } else {
                        offset++;
                    }
                } else {
                    entries.insert(entries.begin() + offset, entry);
                    offset++;
                }
            }
            entries.reserve(entries.size() + other.entries.size() - otherOffset);
            for (; otherOffset < other.entries.size(); otherOffset++) {
                auto entry = other.entries[otherOffset];
                entry.value *= factor;
                entries.push_back(entry);
            }
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
        size_type columnCount = r.columns.size();
        Matrix out;
        out.setEmpty(columnCount);

        for (size_type i = 0; i < columnCount; i++) {
            for (Entry entry : r.columns[i].entries) {
                out.columns[i].add(l.columns[entry.index], entry.value);
            }
        }

        return out;
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

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

        bool operator<(Entry other) const {
            return index < other.index;
        }
        bool operator<(size_type index) const {
            return this->index < index;
        }
    };

    class Column {
    public:
        using iterator = typename std::vector<Entry>::iterator;

        uint64_t mask = 0;

        void addByIndex(size_type index, T value) {
            ZeroTester tester;
            if (tester(value))
                return;
            size_type offset = getAndConstructByIndex(index);
            at(offset).value += value;
            if (tester(at(offset).value)) {
                erase(offset);
            }
        }

        size_type getAndConstructByIndex(size_type index) {
            size_type offset = findByIndex(index);
            if (offset >= size() || at(offset).index != index) {
                insert(offset, Entry(0, index));
            }
            return offset;
        }

        size_type findByIndex(size_type index) {
            return std::lower_bound(entries.begin(), entries.end(), index) - entries.begin();
        }

        Entry& operator[](size_type offset) {
            return entries[offset];
        }

        size_type size() { return entries.size(); }

        Entry& back() { return entries.back(); }

        void insert(size_type offset, Entry entry) {
            mask |= (uint64_t)1 << entry.index;
            insert_internal(offset, entry);
        }

        void insert_internal(size_type offset, Entry entry) {
            entries.insert(entries.begin() + offset, entry);
        }

        void pushBack(Entry entry) {
            entries.push_back(entry);
            mask |= (uint64_t)1 << entry.index;
        }

        void erase(size_type offset) {
            mask &= ~((uint64_t)1 << at(offset).index);
            erase_internal(offset);
        }

        void erase_internal(size_type offset) {
            entries.erase(entries.begin() + offset);
        }

        void eraseAfter(size_type offset) {
            mask &= ((uint64_t)1 << (at(offset).index + 1)) - 1;
            entries.erase(entries.begin() + offset + 1, entries.end());
        }

        bool empty() { return entries.empty(); }

        void clear() {
            mask = 0;
            entries.clear();
        }

        iterator begin() { return entries.begin(); }

        iterator end() { return entries.end(); }

        void reserveForAdd(uint64_t addMask) {
            reserve(size() + std::popcount(addMask & ~mask));
        }

        void reserve(size_type size) {
            entries.reserve(size);
        }

        void add(Column& other, T factor, size_type offset = 0, size_type otherOffset = 0) {
            reserveForAdd(other.mask);
            mask |= other.mask;
            add_internal(other, factor, offset, otherOffset);
        }

        void add_internal(Column& other, T factor, size_type offset = 0, size_type otherOffset = 0) {
            ZeroTester tester;
            for (; otherOffset < other.size(); otherOffset++) {
                auto entry = other[otherOffset];
                entry.value *= factor;
                while (offset < size() && at(offset).index < entry.index) {
                    offset++;
                }
                if (offset >= size()) {
                    break;
                }
                if (at(offset).index == entry.index) {
                    at(offset).value += entry.value;
                    if (tester(at(offset).value)) {
                        erase_internal(offset);
                    } else {
                        offset++;
                    }
                } else {
                    insert_internal(offset, entry);
                    offset++;
                }
            }
            // entries.reserve(size() + other.size() - otherOffset);
            for (; otherOffset < other.size(); otherOffset++) {
                auto entry = other[otherOffset];
                entry.value *= factor;
                pushBack(entry);
            }
        }

    private:
        std::vector<Entry> entries;

        Entry& at(size_type offset) {
            return entries[offset];
        }

        void validateMask(const char* where) {
            uint64_t m = 0;
            for (Entry entry : *this) {
                m |= (uint64_t)1 << entry.index;
            }
            if (m != mask) {
                LOGD("validateMask failed in %s (is %llx shoud be %llx)", where, mask, m);
            }
        }

        void validateOrder(const char* where) {
            if (empty())
                return;
            size_type prev = at(0).index;
            for (size_type i = 1; i < size(); i++) {
                if (prev >= at(i).index) {
                    LOGD("validate order failed in %s", where);
                    return;
                }
                prev = at(i).index;
            }
        }
    };

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

    static Matrix multiply(Matrix& l, Matrix& r) {
        size_type columnCount = r.columns.size();
        Matrix out;
        out.setEmpty(columnCount);

        for (size_type i = 0; i < columnCount; i++) {
            uint64_t mask = 0;
            for (Entry entry : r.columns[i]) {
                mask |= l.columns[entry.index].mask;
            }
            out.columns[i].reserveForAdd(mask);
            out.columns[i].mask = mask;
            for (Entry entry : r.columns[i]) {
                out.columns[i].add_internal(l.columns[entry.index], entry.value);
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
            for (size_type j = 0; j < columnCount; j++) {
                std::cout << mat[i + j * rowCount] << ", ";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
        delete mat;
    }
};

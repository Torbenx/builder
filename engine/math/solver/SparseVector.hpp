#pragma once

#include "math/solver/SparseEntry.hpp"
#include <iostream>

template<typename T, typename size_type, class ZeroTester>
class SparseVector {
public:
    using Entry = SparseEntry<T, size_type>;
    using Vector = SparseVector<T, size_type, ZeroTester>;
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

    T getValueByIndex(size_type index) {
        size_type offset = findByIndex(index);
        if (offset >= size() || at(offset).index != index) {
            return 0;
        }
        return at(offset).value;
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

    void sort() { std::sort(entries.begin(), entries.end()); }

    iterator begin() { return entries.begin(); }

    iterator end() { return entries.end(); }

    void reserveForAdd(uint64_t addMask) {
        reserve(size() + std::popcount(addMask & ~mask));
    }

    void reserve(size_type size) {
        entries.reserve(size);
    }

    void add(Vector& other, T factor, size_type offset = 0, size_type otherOffset = 0, size_type otherEnd = std::numeric_limits<size_type>::max()) {
        reserveForAdd(other.mask);
        mask |= other.mask;
        add_internal(other, factor, offset, otherOffset, otherEnd);
    }

    void add_internal(Vector& other, T factor, size_type offset = 0, size_type otherOffset = 0, size_type otherEnd = std::numeric_limits<size_type>::max()) {
        ZeroTester tester;
        otherEnd = std::min(otherEnd, other.size());
        for (; otherOffset < otherEnd; otherOffset++) {
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
        for (; otherOffset < otherEnd; otherOffset++) {
            auto entry = other[otherOffset];
            entry.value *= factor;
            pushBack(entry);
        }
    }

    void debug() {
        for (auto entry : *this) {
            std::cout << "{" << entry.index << ", " << entry.value << "}, ";
        }
        std::cout << "\n\n";
    }

private:
    std::vector<Entry> entries;

    Entry& at(size_type offset) {
        return entries[offset];
    }
};

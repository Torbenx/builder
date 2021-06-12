#pragma once

template<typename T, typename size_type>
struct SparseEntry {
    using Entry = SparseEntry<T, size_type>;

    SparseEntry(T value, size_type index)
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

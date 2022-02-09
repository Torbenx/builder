#pragma once

#include "util/types.hpp"
#include <limits>
#include <set>

class RangeAllocator {
public:
    using size_type = uint32_t;
    static constexpr size_type INVALID_OFFSET = std::numeric_limits<size_type>::max();
    static constexpr bool isValid(size_type offset) { return offset != INVALID_OFFSET; }
    RangeAllocator(size_type size = 0, size_type max = std::numeric_limits<size_type>::max() / 2);
    size_type allocate(size_type alignment, size_type size);
    void addRange(size_type begin, size_type size);
    void free(size_type start);
    void debug();

private:
    struct Range {
        mutable size_type m_free : 1;
        mutable size_type m_end : 31;

        Range(size_type end, bool free)
            : m_free(free), m_end(end) { }

        size_type end() const { return m_end; }
        bool free() const { return m_free; }
        void setUsed() const { m_free = false; }
        void setFree() const { m_free = true; }

        bool operator<(const Range& other) const { return m_end < other.m_end; }
    };

    std::set<Range, std::less<>> ranges;
};

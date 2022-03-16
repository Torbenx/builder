#include "containers/RangeAllocator.hpp"
#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include <bit>

RangeAllocator::RangeAllocator(size_type size, size_type max) {
    ranges.insert({ max, false });
    if (size > 0)
        ranges.insert({ size, true });
}

RangeAllocator::size_type RangeAllocator::allocate(size_type alignment, size_type size) {
    // VERIFY(Containers, std::has_single_bit(alignment));
    size_type start = 0;
    for (auto it = ranges.begin(); it != ranges.end(); start = it->end(), it++) {
        if (!it->free())
            continue;

        size_type aligned_start = stx::alignmentCeil(alignment, start);
        if (it->end() - aligned_start < size)
            continue;

        if (start != aligned_start) {
            ranges.insert(it, { aligned_start, true });
        }

        if (it->end() != aligned_start + size) {
            Range newRange { aligned_start + size, false };
            ranges.insert(it, newRange);
        } else {
            it->setUsed();
        }

        return aligned_start;
    }
    return INVALID_OFFSET;
}

void RangeAllocator::addRange(size_type begin, size_type size) {
    auto it = ranges.upper_bound({ begin, false });
    VERIFY(Containers, it != ranges.end());
    VERIFY(Containers, !it->free());
    if (it == ranges.begin()) {
        if (begin != 0)
            ranges.insert({ begin, false });
        ranges.insert({ begin + size, true });
    } else {
        auto prev = it;
        prev--;
        if (prev->end() == begin) {
            if (prev->free()) {
                prev->m_end += size;
            } else {
                ranges.insert({ begin + size, true });
            }
        } else {
            ranges.insert({ begin, false });
            ranges.insert({ begin + size, true });
        }
    }
}

void RangeAllocator::debug() {
    size_type start = 0;
    for (Range range : ranges) {
        log<Tag::Containers>("{} {:#x} - {:#x}", range.free() ? 'f' : 'u', start, range.end());
        start = range.end();
    }
}

void RangeAllocator::free(size_type offset) {
    auto it = ranges.upper_bound({ offset, false });
    VERIFY(Containers, it != ranges.end());
    VERIFY(Containers, !it->free());
    it->setFree();
    if (it != ranges.begin()) {
        auto prev = it;
        prev--;
        EXPECT(Containers, offset, prev->end());
        if (prev->free()) {
            ranges.erase(prev);
        }
    } else {
        EXPECT(Containers, offset, (size_type)0);
    }
    auto next = it;
    next++;
    if (next != ranges.end() && next->free()) {
        ranges.erase(it);
    }
}

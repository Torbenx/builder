#include "containers/RangeAllocator.hpp"
#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include <bit>

RangeAllocator::RangeAllocator(size_type size)
    : totalSize(size) {
    if (size > 0)
        ranges.insert({ size, true });
}

RangeAllocator::size_type RangeAllocator::allocate(size_type alignment, size_type size) {
    //VERIFY(Containers, std::has_single_bit(alignment));
    size_type start = 0;
    for (auto it = ranges.begin(); it != ranges.end(); start = it->end(), it++) {
        if (!it->free())
            continue;

        size_type aligned_start = alignmentCeil(alignment, start);
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
    //VERIFY_NOT_REACHED(Containers);
    return INVALID_OFFSET;
}

void RangeAllocator::extendRange(size_type size) {
    ranges.insert({ totalSize + size, true });
    totalSize += size;
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

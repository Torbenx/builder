#pragma once

#include "util/log.hpp"
#include "util/types.hpp"

template<typename mask_t>
struct SmallPoolAllocator {

// one in state indicates free

struct AllocResult {
    // bits to clear in state
    mask_t mask;
    int offset;
};

constexpr mask_t alignmentMasks[] = {
    0xffff'ffff'ffff'ffff,
    0x5555'5555'5555'5555,
    0x1111'1111'1111'1111,
    0x0101'0101'0101'0101,
    0x0001'0001'0001'0001,
    0x0000'0001'0000'0001,
    0x0000'0000'0000'0001
};

int getFreeSlot(mask_t mask) {
    return std::countl_zero(mask);
}

mask_t getMask(int offset, int_t size) {
    mask_t sizeMask = ((mask_t)1 << size) - 1;
    return sizeMask << (offset ^ std::numeric_limits<mask_t>::digits);
}

template<size_t alignment>
constexpr mask_t alignmentMask() {
    mask_t mask = 0;
    for (int_t i = 0; i < std::numeric_limits<mask_t>::digits; i++) {
        mask |= (mask_t)1 << (i * alignment);
    }
    return mask;
}

template<int_t size, int_t stride = 1>
mask_t availableMask(mask_t in) {
    constexpr int_t factor = primeFactor(size);
    constexpr int_t base = size / factor;
    if constexpr (base != 1) {
        return availableMask<factor, stride * base>(availableMask<base, stride>(in));
    } else if constexpr(factor == 2) {
        return (in >> stride) & in;
    } else if constexpr(factor == 3) {
        return (in >> (2 * stride)) & (in >> stride) & in;
    } else {
        mask_t half = availableMask<factor / 2, stride>(in);
        return (in >> ((factor - 1) * stride)) & (half >> ((factor / 2) * stride)) & half;
    }
}

template<size_t alignment, int_t size>
AllocResult allocate(mask_t state) {
    constexpr mask_t alignMask = alignmentMask<alignment>();
    mask_t avail = availableMask<size>(state);
    mask_t valid = avail & alignMask;
    int offset = getFreeSlot(valid);
    return { getMask(offset, size), offset };
}

template<size_t alignment>
AllocResult allocatePowerOf2(int_t exponent, mask_t state) {
    constexpr mask_t alignMask = alignmentMask<alignment>();
    int_t size = (int_t)1 << exponent;
    for (int_t i = 1; i <= exponent; i++)
        state &= state >> i;
    state &= alignMask;
    int offset = getFreeSlot(state);
    return { getMask(offset, size), offset };
}

// returns bits to set in state
template<int_t size>
mask_t freeMask(int_t offset) {
    constexpr mask_t mask = ((uint64_t)1 << size) - 1;
    return getMask(offset, size);
}

}

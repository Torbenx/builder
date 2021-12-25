#pragma once

#include "util/log.hpp"
#include "util/types.hpp"

namespace SmallPoolAllocator {

// one in state indicates free

struct AllocResult {
    // bits to clear in state
    uint64_t mask;
    int offset;
};

constexpr uint64_t alignmentMasks[] = {
    0xffff'ffff'ffff'ffff,
    0x5555'5555'5555'5555,
    0x1111'1111'1111'1111,
    0x0101'0101'0101'0101,
    0x0001'0001'0001'0001,
    0x0000'0001'0000'0001,
    0x0000'0000'0000'0001
};

template<uint64_t size, uint64_t stride = 1>
uint64_t availableMask(uint64_t in) {
    constexpr uint64_t factor = primeFactor(size);
    constexpr uint64_t base = size / factor;
    if constexpr (base != 1) {
        return availableMask<factor, stride * base>(availableMask<base, stride>(in));
    } else if constexpr(factor == 2) {
        return (in >> stride) & in;
    } else if constexpr(factor == 3) {
        return (in >> (2 * stride)) & (in >> stride) & in;
    } else {
        uint64_t half = availableMask<factor / 2, stride>(in);
        return (in >> ((factor - 1) * stride)) & (half >> ((factor / 2) * stride)) & half;
    }
}

template<uint64_t alignment, uint64_t size>
AllocResult allocate(uint64_t state) {
    constexpr uint64_t mask = alignmentMasks[std::countl_zero(alignment)];
    uint64_t avail = availableMask<size>(state);
    uint64_t valid = avail & mask;
    int offset = std::countl_zero(valid);
    return { (uint64_t)1 << offset, offset };
}

template<uint64_t alignment>
AllocResult allocatePowerOf2(int_t exponent, uint64_t state) {
    constexpr uint64_t mask = alignmentMasks[std::countl_zero(alignment)];
    for (int_t i = 1; i <= exponent; i++)
        state &= state >> i;
    int offset = std::countl_zero(state);
    return { (uint64_t)1 << offset, offset };
}

// returns bits to set in state
template<uint64_t size>
uint64_t freeMask(int_t offset) {
    constexpr uint64_t mask = ((uint64_t)1 << size) - 1;
    return mask << offset;
}

}

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

    static constexpr mask_t alignmentMasks[] = {
        0xffff'ffff'ffff'ffff,
        0x5555'5555'5555'5555,
        0x1111'1111'1111'1111,
        0x0101'0101'0101'0101,
        0x0001'0001'0001'0001,
        0x0000'0001'0000'0001,
        0x0000'0000'0000'0001
    };

    static int getFreeSlot(mask_t mask) {
        return std::countl_zero(mask);
    }

    static mask_t getMask(int offset, int_t size) {
        mask_t sizeMask = ((mask_t)1 << size) - 1;
        return sizeMask << (offset ^ (std::numeric_limits<mask_t>::digits - 1));
    }

    template<size_t alignment>
    static constexpr mask_t alignmentMask() {
        mask_t mask = 0;
        for (int_t i = 0; i < std::numeric_limits<mask_t>::digits; i++) {
            mask |= (mask_t)1 << (i * alignment);
        }
        return mask;
    }

    static constexpr int_t primeFactor(int_t size) {
        constexpr int_t primes[] = { 2, 3, 5, 7 };
        for (int_t prime : primes) {
            if (size % prime == 0)
                return prime;
        }
        return 0;
    }

    template<int_t size, int_t stride = 1>
    static mask_t availableMask(mask_t in) {
        constexpr int_t factor = primeFactor(size);
        if constexpr (size == 1) {
            return in;
        } else if constexpr (size > factor) {
            static_assert(factor != 0);
            constexpr int_t base = size / factor;
            return availableMask<factor, stride * base>(availableMask<base, stride>(in));
        } else if constexpr (factor == 2) {
            return (in >> stride) & in;
        } else if constexpr (factor == 3) {
            return (in >> (2 * stride)) & (in >> stride) & in;
        } else {
            mask_t half = availableMask<factor / 2, stride>(in);
            return (in >> ((factor - 1) * stride)) & (half >> ((factor / 2) * stride)) & half;
        }
    }

    template<size_t alignment, int_t size>
    static AllocResult allocate(mask_t state) {
        constexpr mask_t alignMask = alignmentMask<alignment>();
        mask_t avail = availableMask<size>(state);
        mask_t valid = avail & alignMask;
        int offset = getFreeSlot(valid);
        return { getMask(offset, size), offset };
    }

    template<size_t alignment>
    static AllocResult allocatePowerOf2(int_t exponent, mask_t state) {
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
    static mask_t freeMask(int_t offset) {
        constexpr mask_t mask = ((uint64_t)1 << size) - 1;
        return getMask(offset, size);
    }
};

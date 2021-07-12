#pragma once

#include "util/types.hpp"
#include <arm_neon.h>
#include <bit>

template<typename T>
class ArmCacheSet {
private:
    struct IteratorState {
        uint64_t mask;
        uint8_t offset;
        uint8_t tagOffset;
    };

    uint8_t tags[16];
    uint64_t used = 0;
    T values[16];

    static uint64_t usedMask(uint8_t offset) { return ((uint64_t)0xf << 60) >> (offset << 2); }

    static uint8_t computeTagOffset(uint8_t offset) {
        offset = (offset >> 1) | (offset << 3);
        offset = 0xf & ~offset;
        return offset;
    }

    bool isUsed(uint8_t offset) { return used & usedMask(offset); }

    void insert(uint8_t offset, uint8_t tagOffset, uint8_t tag, T value) {
        tags[tagOffset] = tag;
        values[offset] = value;
        used |= usedMask(offset);
    }

public:
    void remove(uint8_t offset) {
        used &= ~usedMask(offset);
    }

    uint8_t getTag(IteratorState state) { return tags[state.tagOffset]; }
    T& getValue(IteratorState state) { return values[state.offset]; }
    void insert(IteratorState state, uint8_t tag, T value) { insert(state.offset, state.tagOffset, tag, value); }

    T& getValue(uint8_t offset) { return values[offset]; }

    IteratorState iterate() { return { used, 0xff }; }

    bool nextEntry(IteratorState& state) {
        if (state.mask == 0)
            return false;
        state.offset = std::countl_zero(state.mask);
        state.mask &= (std::numeric_limits<uint64_t>::max() >> 4) >> state.offset;
        state.offset >>= 2;
        state.tagOffset = computeTagOffset(state.offset);
        return true;
    }

    void insert(uint8_t offset, uint8_t tag, T value) { insert(offset, computeTagOffset(offset), tag, value); }

    // only called when *this is full
    size_t determineGrowth(uint8_t tag) {
        uint8_t mask = 0xff;
        for (uint8_t i = 0; i < 16; i++) {
            mask &= ~(tags[i] ^ tag);
        }
        return std::countr_one(mask) + 1;
    }

    uint8_t matchEmpty() { return std::countl_one(used) >> 2; }

    uint8_t matchTag(uint8_t tag) {
        uint8x16_t v = vld1q_u8(tags);
        uint8x16_t w = vdupq_n_u8(tag);
        v = vceqq_u8(v, w);
        uint8x8_t o = vsli_n_u8(vget_low_u8(v), vget_high_u8(v), 4);
        uint64_t mask = vget_lane_u64((uint64x1_t)o, 0);
        mask &= used;
        return std::countl_zero(mask) >> 2;
    }
};

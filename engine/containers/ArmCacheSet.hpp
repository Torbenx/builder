#pragma once

#include "util/types.hpp"
#include <arm_neon.h>
#include <bit>

template<typename T, int_t D>
class ArmCacheSet {
public:
    static constexpr int_t N = 16;

    uint8_t tags[D][16];
    uint64_t used = 0;
    T values[16];

private:
    struct IteratorState {
        uint64_t mask;
        uint8_t offset;
        uint8_t tagOffset;
    };

    static uint64_t usedMask(uint8_t offset) { return ((uint64_t)0xf << 60) >> (offset << 2); }

    static uint8_t computeTagOffset(uint8_t offset) {
        offset = (offset >> 1) | (offset << 3);
        offset = 0xf & ~offset;
        return offset;
    }

    bool isUsed(uint8_t offset) { return used & usedMask(offset); }

    void insert(uint8_t offset, uint8_t tagOffset, const uint8_t* tag, T value) {
        for (int_t i = 0; i < D; i++) {
            tags[i][tagOffset] = tag[i];
        }
        values[offset] = value;
        used |= usedMask(offset);
    }

public:
    void remove(uint8_t offset) {
        used &= ~usedMask(offset);
    }
    std::array<uint8_t, D> getTag(IteratorState state) {
        std::array<uint8_t, D> tag;
        for (int_t i = 0; i < D; i++) {
            tag[i] = tags[i][state.tagOffset];
        }
        return tag;
    }
    T& getValue(IteratorState state) { return values[state.offset]; }
    void insert(IteratorState state, const uint8_t* tag, T value) { insert(state.offset, state.tagOffset, tag, value); }

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

    void insert(uint8_t offset, const uint8_t* tag, T value) { insert(offset, computeTagOffset(offset), tag, value); }

    uint8_t matchEmpty() { return std::countl_one(used) >> 2; }

    uint8_t matchTag(const uint8_t* tag) {
        uint64_t mask = used;
        for (int_t i = 0; i < D; i++) {
            uint8x16_t v = vld1q_u8(tags[i]);
            uint8x16_t w = vdupq_n_u8(tag[i]);
            v = vceqq_u8(v, w);
            uint8x8_t o = vsli_n_u8(vget_low_u8(v), vget_high_u8(v), 4);
            mask &= vget_lane_u64((uint64x1_t)o, 0);
        }
        return std::countl_zero(mask) >> 2;
    }
};

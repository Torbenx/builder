#pragma once

#include "util/types.hpp"
#include <bit>

template<typename T>
class PortableCacheSet {
private:
    struct IteratorState {
        uint64_t mask;
        uint8_t offset;
    };

    uint8_t tags[16];
    uint64_t used = 0;
    T values[16];

    static uint64_t usedMask(uint8_t offset) { return ((uint64_t)0xf << 60) >> (offset << 2); }

    bool isUsed(uint8_t offset) { return used & usedMask(offset); }

public:
    void remove(uint8_t offset) {
        used &= ~usedMask(offset);
    }

    uint8_t getTag(IteratorState state) { return tags[state.offset]; }
    T& getValue(IteratorState state) { return values[state.offset]; }
    void insert(IteratorState state, uint8_t tag, T value) { insert(state.offset, tag, value); }

    T& getValue(uint8_t offset) { return values[offset]; }

    void insert(uint8_t offset, uint8_t tag, T value) {
        tags[offset] = tag;
        values[offset] = value;
        used |= usedMask(offset);
    }

    IteratorState iterate() { return { used, 0xff }; }

    bool nextEntry(IteratorState& state) {
        if (state.mask == 0)
            return false;
        state.offset = std::countl_zero(state.mask);
        state.mask &= (std::numeric_limits<uint64_t>::max() >> 4) >> state.offset;
        state.offset >>= 2;
        return true;
    }

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
        uint8_t ret = 16;
        for (uint8_t i = 0; i < 16; i++) {
            if (isUsed(i) && tags[i] == tag) {
                ret = i;
            }
        }
        return ret;
    }
};

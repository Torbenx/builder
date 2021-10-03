#pragma once

#include "util/types.hpp"
#include <bit>

template<typename T, int_t D>
class PortableCacheSet {
public:
    static constexpr int_t N = 16;

private:
    struct IteratorState {
        uint64_t mask;
        uint8_t offset;
    };

    uint8_t tags[D][16];
    uint64_t used = 0;
    T values[16];

    static uint64_t usedMask(uint8_t offset) { return ((uint64_t)0xf << 60) >> (offset << 2); }

    bool isUsed(uint8_t offset) { return used & usedMask(offset); }

public:
    void remove(uint8_t offset) {
        used &= ~usedMask(offset);
    }

    std::array<uint8_t, D> getTag(IteratorState state) {
        std::array<uint8_t, D> tag;
        for (int_t i = 0; i < D; i++) {
            tag[i] = tags[i][state.offset];
        }
        return tag;
    }
    T& getValue(IteratorState state) { return values[state.offset]; }
    void insert(IteratorState state, const uint8_t* tag, T value) { insert(state.offset, tag, value); }

    T& getValue(uint8_t offset) { return values[offset]; }

    void insert(uint8_t offset, const uint8_t* tag, T value) {
        for (int_t i = 0; i < D; i++) {
            tags[i][offset] = tag[i];
        }
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
    std::array<uint8_t, D> determineGrowth(const uint8_t* tag) {
        std::array<uint8_t, D> mask { 0xff };
        for (int_t i = 0; i < 16; i++) {
            for (int_t j = 0; j < D; j++) {
                mask[j] &= ~(tags[j][i] ^ tag[j]);
            }
        }
        uint8_t min = std::numeric_limits<uint8_t>::max();
        int_t minIndex;
        for (int_t j = 0; j < D; j++) {
            uint8_t v = std::countr_one(mask[j]) + 1;
            if (v < min) {
                min = v;
                minIndex = j;
            }
        }
        mask.fill(0);
        mask[minIndex] = min;
        return mask;
    }

    uint8_t matchEmpty() { return std::countl_one(used) >> 2; }

    uint8_t matchTag(const uint8_t* tag) {
        uint8_t ret = 16;
        for (uint8_t i = 0; i < 16; i++) {
            bool match = isUsed(i);
            for (int_t j = 0; j < D; j++) {
                if (tags[j][i] != tag[j]) {
                    match = false;
                }
            }
            if (match) {
                ret = i;
            }
        }
        return ret;
    }
};

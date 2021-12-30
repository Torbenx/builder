#pragma once

#include "util/types.hpp"
#include <bit>
#include <immintrin.h>

template<typename T, int_t D, typename mask_t>
class X86CacheSet {
public:
    static constexpr int_t N = sizeof(mask_t) * 8;

    uint8_t tags[D][N];
    mask_t used = 0;
    T values[N];

private:
    struct IteratorState {
        mask_t mask;
        uint8_t offset;
    };

    static mask_t usedMask(uint8_t offset) { return 1 << offset; }

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
        state.offset = std::countr_zero(state.mask);
        state.mask &= (state.mask - 1);
        return true;
    }

    uint8_t matchEmpty() { return std::countr_one(used); }
};

template<typename T, int_t D>
struct SSECacheSet : X86CacheSet<T, D, uint16_t> {

    uint8_t matchTag(const uint8_t* tag) {
        auto mask = this->used;
        for (int_t i = 0; i < D; i++) {
            auto v = _mm_set1_epi8(tag[i]);
            auto w = _mm_loadu_si128((const __m128i*)&this->tags[i]);
            mask &= _mm_movemask_epi8(_mm_cmpeq_epi8(v, w));
        }
        return std::countr_zero(mask);
    }
};

template<typename T, int_t D>
struct AVX2CacheSet : X86CacheSet<T, D, uint32_t> {

    uint8_t matchTag(const uint8_t* tag) {
        auto mask = this->used;
        for (int_t i = 0; i < D; i++) {
            auto v = _mm256_set1_epi8(tag[i]);
            auto w = _mm256_loadu_si256((const __m256i*)&this->tags[i]);
            mask &= _mm256_movemask_epi8(_mm256_cmpeq_epi8(v, w));
        }
        return std::countr_zero(mask);
    }
};

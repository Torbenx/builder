#pragma once

#include <bit>
#include <util/types.hpp>

template<typename T>
class CacheMap {
private:
    struct Set {
        uint8_t tags[16];
        T values[16];
    };

    Set* sets;
    size_t logSize;

public:
    struct Hint {
        Set* set;
        uint8_t offset;

        bool isValid() { return offset < 16; }
        T& operator*() { return set->values[offset]; }
    };

    CacheMap() {
        sets = new Set[1];
        std::memset(sets, 0xff, sizeof(Set));
        logSize = 0;
    }

    ~CacheMap() {
        delete[] sets;
    }

    Hint find(uint16_t key) {
        if (16 - std::countl_zero(key) > logSize + 7) {
            return { nullptr, 16 };
        }
        return findUnchecked(key);
    }

    Hint findUnchecked(uint16_t key) {
        uint16_t index = key & ((1 << logSize) - 1);
        uint8_t tag = key >> logSize;
        Set& set = sets[index];
        return { &set, match(set, tag) };
    }

    void insertOrAssign(uint16_t key, T value) {
        insertOrAssignHint(find(key), key, value);
    }

    void insertOrAssignHint(Hint hint, uint16_t key, T value) {
        if (hint.isValid()) {
            *hint = value;
            return;
        }
        if (hint.set == nullptr) {
            grow((16 - std::countl_zero(key)) - (logSize + 7));
            return insertOrAssign(key, value);
        }
        uint8_t offset = match(*hint.set, 0xff);
        if (offset >= 16) {
            grow(determineGrowth(*hint.set, key >> logSize));
            return insertOrAssign(key, value);
        }
        hint.set->tags[tagOffset(offset)] = key >> logSize;
        hint.set->values[offset] = value;
    }

    uint32_t& operator[](uint16_t key) {
        return *findUnchecked(key);
    }

private:
    // n = number of bits to grow
    void grow(size_t n) {
        LOGD("growing by %llu", n);
        size_t newSize = 1 << (logSize + n);
        Set* newSets = new Set[newSize];
        std::memset(newSets, 0xff, newSize * sizeof(Set));

        for (size_t index = 0; index < (1 << logSize); index++) {
            Set& set = sets[index];

            auto state = iterateSet(set);
            uint8_t offset;
            while ((offset = nextEntry(set, state)) < 16) {
                uint8_t tagOffset = CacheMap::tagOffset(offset);
                uint8_t tag = set.tags[tagOffset];
                size_t indexDiff = tag & ((1 << n) - 1);
                size_t newIndex = index + (indexDiff << logSize);
                LOGD("moving %u, indexDiff=0x%llx, newIndex=%llu", tag, indexDiff, newIndex);
                newSets[newIndex].tags[tagOffset] = tag >> n;
                newSets[newIndex].values[offset] = set.values[offset];
            }
        }

        delete[] sets;
        sets = newSets;
        logSize += n;
    }

    static uint8_t iterateSet(Set& set) {
        return 0xff;
    }

    static uint8_t nextEntry(Set& set, uint8_t& offset) {
        while (true) {
            ++offset;
            if (offset >= 16) {
                return 0xff;
            }
            if ((set.tags[offset] & 0x80) == 0) {
                return offset;
            }
        }
    }

    static size_t determineGrowth(Set& set, uint8_t tag) {
        uint8_t mask = 0xff;
        for (uint8_t i = 0; i < 16; i++) {
            mask &= ~(set.tags[i] ^ tag);
        }
        return std::countr_one(mask) + 1;
    }

    static uint8_t match(Set& set, uint8_t tag) {
        uint8_t ret = 16;
        for (uint8_t i = 0; i < 16; i++) {
            if (set.tags[i] == tag) {
                ret = i;
            }
        }
        return ret;
    }

    static uint8_t tagOffset(uint8_t offset) { return offset; }

    /*uint8x16_t v = vld1q_u8(low);
    uint8x16_t w = vdupq_n_u8(target);
    v = vceqq_u8(v, w);
    uint8x8_t o = vsli_n_u8(vget_low_u8(v), vget_high_u8(v), 4);
    return std::countl_zero(vget_lane_u64((uint64x1_t)o, 0)) >> 2;*/
};

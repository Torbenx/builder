#pragma once

#include <bit>
#include <util/types.hpp>

template<typename T>
class PortableSet {
private:
    struct IteratorState {
        uint8_t offset;
    };

    uint8_t tags[16];
    uint64_t used = 0;
    T values[16];

    static uint64_t usedMask(uint8_t offset) { return (uint64_t)0xf << (offset * 4); }

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

    IteratorState iterate() { return { 0xff }; }

    bool nextEntry(IteratorState& state) {
        while (true) {
            ++state.offset;
            if (state.offset >= 16) {
                return false;
            }
            if (isUsed(state.offset)) {
                return true;
            }
        }
    }

    size_t determineGrowth(uint8_t tag) {
        uint8_t mask = 0xff;
        for (uint8_t i = 0; i < 16; i++) {
            mask &= ~(tags[i] ^ tag);
        }
        return std::countr_one(mask) + 1;
    }

    uint8_t matchEmpty() { return std::countr_one(used) >> 2; }

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

template<typename T, class Set>
class CacheMapBase {
private:
    Set* sets;
    size_t logSize;

public:
    struct Hint {
        Set* set;
        uint8_t offset;

        bool isValid() { return offset < 16; }
        T& operator*() { return set->getValue(offset); }
    };

    CacheMapBase() {
        sets = new Set[1];
        logSize = 0;
    }

    ~CacheMapBase() {
        delete[] sets;
    }

    Hint find(uint16_t key) {
        if (16 - std::countl_zero(key) > logSize + 8) {
            return { nullptr, 16 };
        }
        return findUnchecked(key);
    }

    Hint findUnchecked(uint16_t key) {
        uint16_t index = key & ((1 << logSize) - 1);
        uint8_t tag = key >> logSize;
        Set& set = sets[index];
        return { &set, set.matchTag(tag) };
    }

    void insertOrAssign(uint16_t key, T value) { insertOrAssignHint(find(key), key, value); }

    bool has(uint16_t key) { return find(key).isValid(); }

    bool remove(uint16_t key) {
        Hint hint = find(key);
        if (!hint.isValid()) {
            return false;
        }
        removeHint(hint);
        return true;
    }

    void removeUnchecked(uint16_t key) { removeHint(findUnchecked(key)); }

    // hint must be valid
    void removeHint(Hint hint) {
        hint.set->remove(hint.offset);
    }

    void insertOrAssignHint(Hint hint, uint16_t key, T value) {
        if (hint.isValid()) {
            *hint = value;
            return;
        }
        if (hint.set == nullptr) {
            grow((16 - std::countl_zero(key)) - (logSize + 8));
            insertOrAssign(key, value);
            return;
        }
        uint8_t offset = hint.set->matchEmpty();
        if (offset >= 16) {
            grow(hint.set->determineGrowth(key >> logSize));
            insertOrAssign(key, value);
            return;
        }
        hint.set->insert(offset, key >> logSize, value);
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

        for (size_t index = 0; index < (1 << logSize); index++) {
            Set& set = sets[index];

            auto state = set.iterate();
            while (set.nextEntry(state)) {
                uint8_t tag = set.getTag(state);
                size_t indexDiff = tag & ((1 << n) - 1);
                size_t newIndex = index + (indexDiff << logSize);
                LOGD("moving %u, indexDiff=0x%llx, newIndex=%llu", tag, indexDiff, newIndex);
                newSets[newIndex].insert(state, tag >> n, set.getValue(state));
            }
        }

        delete[] sets;
        sets = newSets;
        logSize += n;
    }

    /*uint8x16_t v = vld1q_u8(low);
    uint8x16_t w = vdupq_n_u8(target);
    v = vceqq_u8(v, w);
    uint8x8_t o = vsli_n_u8(vget_low_u8(v), vget_high_u8(v), 4);
    return std::countl_zero(vget_lane_u64((uint64x1_t)o, 0)) >> 2;*/
};

template<typename T>
using CacheMap = CacheMapBase<T, PortableSet<T>>;

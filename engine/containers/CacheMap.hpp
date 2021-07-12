#pragma once

#include "util/types.hpp"
#include "util/target.hpp"

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
};

#ifdef ARCHITECTURE_ARM_64

#include "containers/ArmCacheSet.hpp"
template<typename T>
using CacheMap = CacheMapBase<T, ArmCacheSet<T>>;

#else

#include "containers/PortableCacheSet.hpp"
template<typename T>
using CacheMap = CacheMapBase<T, PortableCacheSet<T>>;

#endif

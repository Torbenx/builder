#pragma once

#include "util/target.hpp"
#include "util/types.hpp"

template<typename T, class Set, int_t D>
class CacheMapBase {
private:
    Set* sets;
    uint8_t logSize[D];

public:
    static constexpr int_t N = Set::N;
    using key_t = uint16_t;

    struct Hint {
        Set* set;
        uint8_t offset;

        bool isValid() { return offset < N; }
        T& operator*() { return set->getValue(offset); }
    };

    CacheMapBase() {
        sets = new Set[1];
        for (int_t i = 0; i < D; i++) {
            logSize[i] = 0;
        }
    }

    ~CacheMapBase() {
        delete[] sets;
    }

    Hint find(const key_t* key) {
        for (int_t i = 0; i < D; i++) {
            if (sizeof(key_t) * 8 - std::countl_zero(key[i]) > logSize[i] + 8) {
                return { nullptr, N };
            }
        }
        return findUnchecked(key);
    }
    Hint find(std::array<key_t, D> key) { return find(key.data()); }

    Hint findUnchecked(const key_t* key) {
        int_t index = key[0] & ((1 << logSize[0]) - 1);
        uint8_t tag[D];
        tag[0] = key[0] >> logSize[0];
        for (int_t i = 1; i < D; i++) {
            index <<= logSize[i];
            index += key[i] & ((1 << logSize[i]) - 1);
            tag[i] = key[i] >> logSize[i];
        }
        Set& set = sets[index];
        return { &set, set.matchTag(tag) };
    }
    Hint findUnchecked(std::array<key_t, D> key) { return findUnchecked(key.data); }

    void insertOrAssign(const key_t* key, T value) { insertOrAssignHint(find(key), key, value); }
    void insertOrAssign(std::array<key_t, D> key, T value) { insertOrAssign(key.data(), value); }

    bool has(const key_t* key) { return find(key).isValid(); }
    bool has(std::array<key_t, D> key) { return has(key.data()); }

    bool remove(const key_t* key) {
        Hint hint = find(key);
        if (!hint.isValid()) {
            return false;
        }
        removeHint(hint);
        return true;
    }
    bool remove(std::array<key_t, D> key) { return remove(key.data()); }

    void removeUnchecked(const key_t* key) { removeHint(findUnchecked(key)); }
    bool removeUnchecked(std::array<key_t, D> key) { return removeUnchecked(key.data()); }

    // hint must be valid
    void removeHint(Hint hint) {
        hint.set->remove(hint.offset);
    }

    void insertOrAssignHint(Hint hint, const key_t* key, T value) {
        if (hint.isValid()) {
            *hint = value;
            return;
        }
        if (hint.set == nullptr) {
            uint8_t n[D];
            for (int_t i = 0; i < D; i++) {
                n[i] = (sizeof(key_t) * 8 - std::countl_zero(key[i])) - (logSize[i] + 8);
            }
            grow(n);
            insertOrAssign(key, value);
            return;
        }
        uint8_t offset = hint.set->matchEmpty();
        uint8_t tag[D];
        for (int_t i = 0; i < D; i++) {
            tag[i] = key[i] >> logSize[i];
        }
        if (offset >= N) {
            grow(hint.set->determineGrowth(tag).data());
            insertOrAssign(key, value);
            return;
        }
        hint.set->insert(offset, tag, value);
    }
    void insertOrAssignHint(Hint hint, std::array<key_t, D> key, T value) { insertOrAssignHint(hint, key, value); }

    T& operator[](const key_t* key) { return *findUnchecked(key); }
    T& operator[](std::array<key_t, D> key) { return (*this)[key.data()]; }

private:
    // n = number of bits to grow
    void grow(const uint8_t* n) {
        size_t newSize = 1;
        size_t oldSize = 1;
        for (int_t i = 0; i < D; i++) {
            oldSize <<= logSize[i];
            newSize <<= logSize[i] + n[i];
        }
        Set* newSets = new Set[newSize];

        for (int_t index = 0; index < oldSize; index++) {
            Set& set = sets[index];

            uint8_t coords[D];
            size_t temp = index;
            for (int_t i = D - 1; i >= 0; i--) {
                coords[i] = temp & ((1 << logSize[i]) - 1);
                temp >>= logSize[i];
            }

            auto state = set.iterate();
            while (set.nextEntry(state)) {
                std::array<uint8_t, D> tag = set.getTag(state);
                int_t newIndex = coords[0];
                newIndex += (tag[0] & ((1 << n[0]) - 1)) << logSize[0];
                tag[0] >>= n[0];
                for (int_t i = 1; i < D; i++) {
                    newIndex <<= logSize[i] + n[i];
                    newIndex += coords[i];
                    newIndex += (tag[i] & ((1 << n[i]) - 1)) << logSize[i];
                    tag[i] >>= n[i];
                }
                newSets[newIndex].insert(state, tag.data(), set.getValue(state));
            }
        }

        for (int_t i = 0; i < D; i++) {
            logSize[i] += n[i];
        }
        delete[] sets;
        sets = newSets;
    }
};

#if defined(ARCHITECTURE_ARM_64)

#include "containers/ArmCacheSet.hpp"
template<typename T, int_t D>
using CacheMap = CacheMapBase<T, ArmCacheSet<T, D>, D>;

#elif defined(ARCHITECTURE_X86_64)

#include "containers/X86CacheSet.hpp"
template<typename T, int_t D>
using CacheMap = CacheMapBase<T, AVX2CacheSet<T, D>, D>;

#else

#include "containers/PortableCacheSet.hpp"
template<typename T, int_t D>
using CacheMap = CacheMapBase<T, PortableCacheSet<T, D>, D>;

#endif

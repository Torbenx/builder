#pragma once

#include "containers/Allocator32Bit.hpp"
#include "util/target.hpp"
#include <atomic>
#include <bit>
#include <memory>

template<typename T, size_t MaxSize>
class StableVector {
public:
    T& insertOrGet(size_t i) {
        ASSUME(i != 0);
        auto j = usedBits(i);
        if (!m_data[j]) [[unlikely]]
            allocate(j);
        return m_data[j][i ^ ((size_t)1 << j)];
    }

    T& operator[](size_t i) {
        ASSUME(i != 0);
        auto j = usedBits(i);
        return m_data[j][i ^ (((size_t)1 << j))];
    }

private:
    static constexpr auto usedBits(size_t i) {
        ASSUME(i != 0);
        return (sizeof(size_t) * CHAR_BIT - 1) ^ (size_t)std::countl_zero(i);
    }

    void allocate(int j) {
        m_data[j].reset(new T[(size_t)1 << j]);
    }

    static constexpr auto MAX_BITS = usedBits(MaxSize - 1);

    std::unique_ptr<T[]> m_data[MAX_BITS + 1] = {};
};

template<typename T, size_t MaxSize, Allocator32Bit Allocator>
class AtomicStableVector {
public:
    AtomicStableVector(Allocator allocator)
        : allocator(allocator) { }

    AtomicStableVector() requires std::is_default_constructible_v<Allocator> { }

    void ensureExists(size_t i) {
        ASSUME(i != 0);
        auto j = usedBits(i);
        if (!m_data[j].load(std::memory_order_relaxed))
            allocate(j);
    }

    Ptr32<T> operator[](size_t i) {
        ASSUME(i != 0);
        auto j = usedBits(i);
        return m_data[j].load(std::memory_order_relaxed).get(i ^ (((size_t)1 << j)));
    }

    ~AtomicStableVector() {
        for (int_t i = 0; i <= MAX_BITS; i++) {
            allocator.free32(m_data[i].load(std::memory_order_relaxed).offset);
        }
    }

private:
    static constexpr auto usedBits(size_t i) {
        ASSUME(i != 0);
        return (sizeof(size_t) * CHAR_BIT - 1) ^ (size_t)std::countl_zero(i);
    }

    void allocate(int j) {
        Ptr32<T[]> ptr = make32<T[]>(allocator, (size_t)1 << j);
        Ptr32<T[]> p;
        if (!m_data[j].compare_exchange_strong(p, ptr, std::memory_order_relaxed)) {
            allocator.free32(ptr.offset);
        }
    }

    static constexpr auto MAX_BITS = usedBits(MaxSize - 1);

    std::atomic<Ptr32<T[]>> m_data[MAX_BITS + 1] = {};

public:
    [[no_unique_address]] Allocator allocator;
};
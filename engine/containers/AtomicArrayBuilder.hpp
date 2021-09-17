#pragma once

#include "containers/ArrayView.hpp"
#include "util/target.hpp"
#include <atomic>
#include <mutex>

template<typename T, size_t block_capacity = (3 * platform::CACHELINE_SIZE - sizeof(void*)) / sizeof(T)>
class AtomicArrayBuilder {
public:
    static_assert(sizeof(void*) == sizeof(std::atomic<uintptr_t>));

    void pushBack(T item) {
        while(true) {
            uintptr_t pos = position.fetch_add(1, std::memory_order_relaxed);
            uintptr_t offset = pos & OFFSET_MASK;
            if (offset < block_capacity) {
                T* ptr = (T*)(pos & PTR_MASK);
                ptr[offset] = item;
                return;
            }
            allocate();
        }
    }

private:
    static constexpr uintptr_t BLOCK_ALIGNMENT = std::bit_ceil(block_capacity * sizeof(T));
    static constexpr uintptr_t OFFSET_MASK = BLOCK_ALIGNMENT - 1;
    static constexpr uintptr_t PTR_MASK = ~OFFSET_MASK;

    struct Block {
        T data[block_capacity];
        Block* next = nullptr;
    };
    typedef ALIGN_AS(BLOCK_ALIGNMENT) Block AlignedBlock;

    alignas(BLOCK_ALIGNMENT) T inline_storage[block_capacity];
    std::atomic<uintptr_t> position = (uintptr_t)inline_storage;
    alignas(platform::CACHELINE_SIZE) std::mutex allocationLock;
    Block* nextBlock = nullptr;
    Block** nextPtr = &nextBlock;

    void allocate() {
        std::lock_guard<std::mutex> g(allocationLock);
        if ((position.load(std::memory_order_relaxed) & OFFSET_MASK) < block_capacity) {
            return;
        }
        Block* newBlock = new AlignedBlock;
        *nextPtr = newBlock;
        nextPtr = &newBlock->next;
        position.store((uintptr_t)(&newBlock->data));
    }
};

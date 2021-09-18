#pragma once

#include "containers/ArrayView.hpp"
#include "util/target.hpp"
#include <atomic>
#include <mutex>
#include <bit>

template<typename T, size_t block_capacity = (3 * platform::cachelineSize() - sizeof(void*)) / sizeof(T)>
class AtomicArrayBuilder {
public:
    static_assert(sizeof(void*) == sizeof(std::atomic<uintptr_t>));

    static constexpr uintptr_t BLOCK_ALIGNMENT = std::bit_ceil(block_capacity * sizeof(T));
    static constexpr uintptr_t OFFSET_MASK = BLOCK_ALIGNMENT - 1;
    static constexpr uintptr_t PTR_MASK = ~OFFSET_MASK;
    static constexpr size_t BLOCK_CAPACITY = block_capacity;

    struct Block {
        T data[block_capacity];
        Block* next = nullptr;
    };
    typedef ALIGN_AS(BLOCK_ALIGNMENT) Block AlignedBlock;

    struct EndSentinel { };
    template<bool Delete>
    struct BlockIterator {
        T* data;
        AlignedBlock* current;
        AlignedBlock* next;
        int_t finalBlockSize;

        BlockIterator& operator++() {
            if constexpr (Delete) {
                delete current;
            }
            if (next != nullptr) {
                data = next->data;
                current = next;
                next = next->next;
            } else {
                data = nullptr;
            }
        }

        ArrayView<T> operator*() {
            return { data, isFinal() ? finalBlockSize : block_capacity };
        }

        bool isFinal() { return next == nullptr; }
        bool operator!=(EndSentinel) { return data != nullptr; }
    };

    void pushBack(T item) {
        while (true) {
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

    template<bool Delete>
    auto blocksUnsafe() {
        AlignedBlock* next = nextBlock;
        int_t finalSize = position.load(std::memory_order_relaxed) & OFFSET_MASK;
        if constexpr (Delete) {
            nextBlock = nullptr;
            position.store((uintptr_t)inline_storage);
        }
        struct {
            BlockIterator<Delete> it;
            BlockIterator<Delete> begin() { return it; }
            EndSentinel end() { return {}; }
        } r { { inline_storage, nullptr, next, finalSize } };
        return r;
    }

    auto getBlocksUnsafe() { return blocksUnsafe<false>(); }
    auto takeBlocksUnsafe() { return blocksUnsafe<true>(); } // must iterate through all blocks to deallocate them!

private:
    alignas(BLOCK_ALIGNMENT) T inline_storage[block_capacity];
    std::atomic<uintptr_t> position = (uintptr_t)inline_storage;
    alignas(platform::cachelineSize()) std::mutex allocationLock;
    AlignedBlock* nextBlock = nullptr;
    AlignedBlock** nextPtr = &nextBlock;

    void allocate() {
        std::lock_guard<std::mutex> g(allocationLock);
        if ((position.load(std::memory_order_relaxed) & OFFSET_MASK) < block_capacity) {
            return;
        }
        AlignedBlock* newBlock = new AlignedBlock;
        *nextPtr = newBlock;
        nextPtr = &newBlock->next;
        position.store((uintptr_t)(&newBlock->data));
    }
};

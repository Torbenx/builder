#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/ArrayView.hpp"
#include "util/target.hpp"
#include <atomic>
#include <bit>
#include <mutex>

template<typename T, Allocator32Bit Allocator, size_t inline_capacity = (3 * platform::cachelineSize() - sizeof(uint64_t)) / sizeof(T)>
class AtomicArrayBuilder {
public:
    static constexpr size_t INLINE_CAPACITY = inline_capacity;
    static constexpr uint64_t OFFSET_MASK = std::numeric_limits<uint32_t>::max();

    struct Block {
        Ptr32<Block> next = nullptr;
        T data[inline_capacity];
    };

    struct EndSentinel { };
    template<bool Delete>
    struct BlockIterator {
        Allocator allocator;
        T* data;
        Ptr32<Block> current;
        Ptr32<Block> next;
        int_t finalBlockSize;

        BlockIterator& operator++() {
            if constexpr (Delete) {
                allocator.free32(current.offset);
            }
            if (next) {
                data = next.get(allocator)->data;
                current = next;
                next = next.get(allocator)->next;
            } else {
                data = nullptr;
            }
        }

        ArrayView<T> operator*() {
            return { data, isFinal() ? finalBlockSize : inline_capacity };
        }

        bool isFinal() { return next == nullptr; }
        bool operator!=(EndSentinel) { return data != nullptr; }
    };

    AtomicArrayBuilder(Allocator allocator)
        : allocator(allocator) {
        VERIFY(Containers, allocator.ownsAddress(this));
        reset();
    }

    void pushBack(T item) {
        while (true) {
            uint64_t pos = position.fetch_add(1, std::memory_order_relaxed);
            uint32_t storage = pos >> 32;
            size_t offset = pos & OFFSET_MASK;
            if (offset < inline_capacity) {
                ((T*)allocator.address(storage))[offset] = item;
                return;
            }
            allocate();
        }
    }

    template<bool Delete>
    auto blocksUnsafe() {
        Ptr32<Block> next = nextBlock;
        int_t finalSize = position.load(std::memory_order_relaxed) & OFFSET_MASK;
        if constexpr (Delete) {
            reset();
        }
        struct {
            BlockIterator<Delete> it;
            BlockIterator<Delete> begin() { return it; }
            EndSentinel end() { return {}; }
        } r { { allocator, inline_storage, nullptr, next, finalSize } };
        return r;
    }

    auto getBlocksUnsafe() { return blocksUnsafe<false>(); }
    auto takeBlocksUnsafe() { return blocksUnsafe<true>(); } // must iterate through all blocks to deallocate them!

private:
    std::atomic<uint64_t> position = 0;
    T inline_storage[inline_capacity];
    alignas(platform::cachelineSize()) std::mutex allocationLock;
    Ptr32<Block> nextBlock;
    Ptr32<Ptr32<Block>> nextPtr;

    void allocate() {
        std::lock_guard<std::mutex> g(allocationLock);
        if ((position.load(std::memory_order_relaxed) & OFFSET_MASK) < inline_capacity) {
            return;
        }
        Ptr32<Block> newBlock = allocator.allocate32(sizeof(Block));
        *nextPtr = newBlock;
        nextPtr = newBlock.offset + offsetof(Block, next);
        position.store((uint64_t)(newBlock.offset + offsetof(Block, data)) << 32);
    }

    void reset() {
        nextBlock = nullptr;
        nextPtr = allocator.offset(&nextBlock);
        position.store((uint64_t)allocator.offset(&inline_storage) << 32);
    }

public:
    Allocator allocator;
};

#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/ArrayView.hpp"
#include "util/target.hpp"
#include "util/types.hpp"
#include <atomic>
#include <bit>
#include <mutex>

namespace detail {
template<typename T>
constexpr size_t generationSize(size_t generation) {
    if (generation == 0)
        return 0;
    return (1 << generation) * 2;
}
};

template<typename T, Allocator32Bit Allocator>
class AtomicArrayBuilder {
public:
    static constexpr size_t GENERATION_BITS = 11;
    static constexpr size_t MAX_GENERATION = ((size_t)1 << GENERATION_BITS) - 1;
    static constexpr size_t GENERATION_SHIFT = 32 - GENERATION_BITS;
    static constexpr uint64_t OFFSET_MASK = ((uint64_t)1 << GENERATION_SHIFT) - 1;
    static constexpr uint64_t GENERATION_MASK = (((uint64_t)1 << 32) - 1) & ~OFFSET_MASK;

    struct BlockHeader {
        Ptr32<BlockHeader> next;
    };
    static constexpr size_t BLOCK_HEADER_SIZE = alignmentCeil(alignof(T), sizeof(BlockHeader));

    struct EndSentinel { };
    struct BlockIterator {
        Allocator allocator;
        Ptr32<BlockHeader> current;
        int_t finalBlockSize;
        size_t generation = 0;

        BlockIterator& operator++() {
            allocator.free32(current.offset);

            current = current.get(allocator)->next;
            generation = std::min(MAX_GENERATION, generation + 1);
        }

        ArrayView<T> operator*() {
            return { (T*)allocator.address(current.offset + BLOCK_HEADER_SIZE), isFinal() ? finalBlockSize : detail::generationSize<T>(generation) };
        }

        bool isFinal() { return current.get(allocator)->next == nullptr; }
        bool operator!=(EndSentinel) { return current != nullptr; }
    };

    AtomicArrayBuilder(Allocator allocator)
        : allocator(allocator) { }

    void pushBack(T item) {
        while (true) {
            Info i = position.fetch_add(1, std::memory_order_relaxed);
            if (i.offset < detail::generationSize<T>(i.generation)) {
                ((T*)allocator.address(i.storage))[i.offset] = item;
                return;
            }
            allocate();
        }
    }

    // must iterate through all blocks to deallocate them!
    BlockIterator take() {
        Ptr32<BlockHeader> headerPtr;
        uint64_t pos;
        {
            std::lock_guard<std::mutex> g(allocationLock);
            pos = position.exchange(initialPosition(), std::memory_order_relaxed);
            headerPtr = firstHeader;
            firstHeader = nullptr;
            latestHeader = nullptr;
        }
        struct {
            BlockIterator it;
            BlockIterator begin() { return it; }
            EndSentinel end() { return {}; }
        } r { { allocator, headerPtr, Info(pos).offset } };
        return r;
    }

    void clear() {
        for (auto _ : take()) { }
    }

    ~AtomicArrayBuilder() {
        clear();
    }

private:
    struct Info {
        Info(uint64_t pos)
            : offset(pos & OFFSET_MASK)
            , generation((pos & GENERATION_MASK) >> GENERATION_SHIFT)
            , storage(pos >> 32) { }
        size_t offset;
        size_t generation;
        uint32_t storage;

        uint64_t pos() {
            return ((uint64_t)storage << 32)
                | ((uint64_t)generation << GENERATION_SHIFT)
                | (uint64_t)offset;
        }
    };

    std::atomic<uint64_t> position = initialPosition();
    std::mutex allocationLock;
    Ptr32<BlockHeader> firstHeader = nullptr;
    Ptr32<BlockHeader> latestHeader = nullptr;

    void allocate() {
        std::lock_guard<std::mutex> g(allocationLock);
        Info i = position.load(std::memory_order_relaxed);
        if (i.offset < detail::generationSize<T>(i.generation)) {
            return;
        }
        i.generation = std::min(MAX_GENERATION, i.generation + 1);
        i.offset = 0;
        size_t dataSize = detail::generationSize<T>(i.generation) * sizeof(T);
        size_t alignment = std::max(alignof(BlockHeader), alignof(T));
        uint32_t newBlock = allocator.allocate32(alignment, BLOCK_HEADER_SIZE + dataSize);
        i.storage = newBlock + BLOCK_HEADER_SIZE;
        Ptr32<BlockHeader> newHeaderPtr = newBlock;
        newHeaderPtr.get(allocator)->next = nullptr;
        latestHeader.get(allocator)->next = newHeaderPtr;
        latestHeader = newHeaderPtr;
        if (!firstHeader) {
            firstHeader = newHeaderPtr;
        }
        position.store(i.pos(), std::memory_order_relaxed);
    }

    uint64_t initialPosition() { return 0; }

public:
    Allocator allocator;
};

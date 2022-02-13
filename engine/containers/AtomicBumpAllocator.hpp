#pragma once

#include "containers/ArrayView.hpp"
#include "memory/SubAllocator.hpp"
#include "util/log.hpp"
#include <atomic>

template<typename Callback, uint32_t defaultAlignment>
class AtomicBumpAllocator {
public:
    AtomicBumpAllocator(uint32_t initialCapacity, uint32_t initialOffset, Callback callback, std::integral_constant<uint32_t, defaultAlignment> = {})
        : capacity(initialCapacity), bump(initialOffset), callback(callback) { }

    uint32_t allocate(size_t alignment, size_t size) {
        if (alignment > defaultAlignment) {
            size += alignment;
        }
        size = alignmentCeil(defaultAlignment, size);

        uint32_t offset = bump.fetch_add(size, std::memory_order_relaxed);
        while (true) {
            uint32_t capacity = this->capacity.load(std::memory_order_acquire);

            if (offset + size <= capacity)
                break;

            if (offset <= capacity)
                extend(capacity);
        }
        return alignmentCeil(alignment, offset);
    }

private:
    std::atomic<uint32_t> capacity;
    std::atomic<uint32_t> bump;
    [[no_unique_address]] Callback callback;

    void extend(uint32_t oldCapacity) {
        uint32_t newCapacity = callback(oldCapacity);
        VERIFY(Containers, newCapacity > oldCapacity);
        capacity.store(newCapacity, std::memory_order_release);
    }
};

template<typename BaseAllocator, size_t defaultAlignment, int_t blockSize>
struct ContinuousAtomicBumpAllocator : SubAllocatorBase<ContinuousAtomicBumpAllocator<BaseAllocator, defaultAlignment, blockSize>, BaseAllocator> {
    using SubAllocatorBase = SubAllocatorBase<ContinuousAtomicBumpAllocator<BaseAllocator, defaultAlignment, blockSize>, BaseAllocator>;
    using uintptr_t = typename SubAllocatorBase::uintptr_t;
    template<typename T = void>
    using Ptr = typename SubAllocatorBase::template Ptr<T>;

    struct Data {
        Data(uintptr_t begin, size_t initialSize = 0)
            : begin(begin), end(begin + initialSize) { }
        std::atomic<uintptr_t> begin = 0;
        std::atomic<uintptr_t> end = 0;
    };

    Data& data;

    ContinuousAtomicBumpAllocator(Data& data, BaseAllocator allocator)
        : SubAllocatorBase(allocator), data(data) { }

    Ptr<> allocateImpl(size_t alignment, int_t size) {
        if (alignment > defaultAlignment) {
            size += alignment;
        }
        size = alignmentCeil(defaultAlignment, size);

        uintptr_t offset = data.begin.fetch_add(size, std::memory_order_relaxed);
        while (true) {
            uintptr_t end = data.end.load(std::memory_order_acquire);

            if (offset + size <= end)
                break;

            // ensure a new block is allocated exactly once
            if (offset <= end) {
                Ptr<> p = this->baseAllocator.template allocate<defaultAlignment, blockSize>();
                EXPECT(Containers, p.addr(), end);
                data.end.store(end + blockSize, std::memory_order_release);
            }
        }
        return { (uintptr_t)alignmentCeil(alignment, offset) };
    }

    void freeImpl([[maybe_unused]] size_t alignment, [[maybe_unused]] Ptr<> ptr, [[maybe_unused]] int_t size) { }
};

template<typename BaseAllocator>
struct PageBlockAllocator : SubAllocatorBase<PageBlockAllocator<BaseAllocator>, BaseAllocator> {
    using SubAllocatorBase = SubAllocatorBase<PageBlockAllocator<BaseAllocator>, BaseAllocator>;
    using uintptr_t = typename SubAllocatorBase::uintptr_t;
    template<typename T = void>
    using Ptr = typename SubAllocatorBase::template Ptr<T>;

    static constexpr int_t LOG_BLOCK_SIZE = 12;
    static constexpr int_t BLOCK_SIZE = (int_t)1 << LOG_BLOCK_SIZE;

    struct Data {
        int_t blockCounter = 0;

        int_t skipPage(int_t count = 1) {
            int_t blocksSkipped = std::max<int_t>((count << platform::logPageSize()) >> LOG_BLOCK_SIZE, 1);
            int_t sizeSkipped = blocksSkipped * BLOCK_SIZE;
            blockCounter += blocksSkipped;
            return sizeSkipped;
        }
    };

    Data& data;

    PageBlockAllocator(Data& data, BaseAllocator allocator)
        : SubAllocatorBase(allocator), data(data) { }

    template<size_t alignment, int_t size>
    Ptr<> allocate() {
        static_assert(size == BLOCK_SIZE);
        auto logPageSize = platform::logPageSize();
        std::byte* beginAddress = (std::byte*)this->baseAllocator.bumpBeginAddress();
        if (LOG_BLOCK_SIZE >= logPageSize) {
            platform::commitMemory(beginAddress + (data.blockCounter << LOG_BLOCK_SIZE), BLOCK_SIZE);
        } else {
            int_t logFactor = logPageSize - LOG_BLOCK_SIZE;
            size_t mask = ((size_t)1 << logFactor) - 1;
            if ((data.blockCounter & mask) == 0)
                platform::commitMemory(beginAddress + (data.blockCounter << LOG_BLOCK_SIZE), (size_t)1 << logPageSize);
        }
        // VERIFY(Allocators, data.blockCounter < data.maxBlocks);
        uintptr_t address = this->baseAllocator.internalBumpBeginAddress() + data.blockCounter * BLOCK_SIZE;
        data.blockCounter += 1;
        return { address };
    }
};

template<typename BlockAllocator, size_t defaultAlignment>
using ContinuousAtomicBumpSubAllocator = ContinuousAtomicBumpAllocator<BlockAllocator, defaultAlignment, BlockAllocator::BLOCK_SIZE>;

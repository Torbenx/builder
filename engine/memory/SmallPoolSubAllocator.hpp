#pragma once

#include "memory/Allocator.hpp"

template<size_t slabAlignment, int_t slabSize, size_t blockAlignment, typename BlockAllocator>
class SmallPoolSubAllocator : public AllocatorBase<BlockSubAllocator> {
public:
    template<typename T = void>
    using Ptr = BlockAllocator::Ptr<T>;
    using uintptr_t = BlockAllocator::uintptr_t;

    struct Data {
        Ptr<Block> first = nullptr;
        Ptr<Block> current = nullptr;
    };

    BlockSubAllocator(Data& data, BlockAllocator blockAllocator)
        : data(data), blockAllocator(blockAllocator) { }

    template<size_t alignment, int_t size>
    Ptr<> allocate() {

    }

private:
    struct Block;
    struct BlockHeader {
        uint64_t state;
        Ptr<Block> next;
    };
    static constexpr int_t HEADER_SIZE_IN_SLABS = (sizeof(BlockHeader) + slabSize - 1) / slabSize;
    struct alignas(slabAlignment) Slab {
        std::byte bytes[slabSize];
    };
    struct alignas(blockAlignment) Block {
        Slab slabs[64 - HEADER_SIZE_IN_SLABS];
        BlockHeader header;
    };

    Data& data;
    BlockAllocator blockAllocator;
};

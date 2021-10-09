#pragma once

#include "containers/RangeAllocator.hpp"
#include "util/types.hpp"

class MemoryAllocator32 {
public:
    MemoryAllocator32(uint32_t maxSize = (uint32_t)1024 * (uint32_t)1024 * (uint32_t)1024 /* 1 GiB */);
    ~MemoryAllocator32();

    [[nodiscard]] uint32_t allocate32(uint32_t alignment, uint32_t size);
    [[nodiscard]] uint32_t allocate32(uint32_t size) { return allocate32(__STDCPP_DEFAULT_NEW_ALIGNMENT__, size); }
    void free32(uint32_t offset);
    void* baseAddress() { return baseAddr; }
    void* address(uint32_t offset) { return baseAddr + offset; }
    bool ownsAddress(void* addr) { return baseAddr <= (std::byte*)addr && (std::byte*)addr - baseAddr < maxSize; }
    uint32_t offset(void* addr) { return (std::byte*)addr - baseAddr; }
    void debug() { allocator.debug(); }

private:
    RangeAllocator allocator;
    std::byte* baseAddr;
    uint32_t maxSize;
};
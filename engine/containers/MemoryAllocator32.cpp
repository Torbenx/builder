#include "containers/MemoryAllocator32.hpp"
#include "containers/ArrayView.hpp"
#include "util/log.hpp"
#include "util/target.hpp"
#include <algorithm>
#include <bit>

MemoryAllocator32::MemoryAllocator32(uint32_t maxSize)
    : allocator(platform::pageSize()), maxSize(maxSize) {
    VERIFY(Containers, maxSize % platform::pageSize() == 0);
    VERIFY(Containers, maxSize > platform::pageSize());
    baseAddr = (std::byte*)platform::reserveAddressSpace(maxSize);
    allocator.allocate(1, platform::pageSize());
}

uint32_t MemoryAllocator32::allocate(uint32_t alignment, uint32_t size) {
    uint32_t offset = allocator.allocate(alignment, size);
    if (allocator.isValid(offset)) {
        return offset;
    }
    size_t pageSize = platform::pageSize();
    size_t sizeToAllocate = ((size + pageSize - 1) / pageSize) * pageSize;
    platform::commitMemory(baseAddr + allocator.rangeSize(), sizeToAllocate);
    allocator.extendRange(sizeToAllocate);
    return allocator.allocate(alignment, size);
}

void MemoryAllocator32::free(uint32_t offset) {
    allocator.free(offset);
}

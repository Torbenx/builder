#pragma once

#include "containers/ArrayView.hpp"
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
#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/ArrayView.hpp"
#include "util/target.hpp"
#include "util/types.hpp"
#include <atomic>
#include <bit>
#include <mutex>

template<typename T>
concept HalfAtomicArrayBuilderAllocator = Allocator32Bit<T> && Allocator32Bit<T::CopyType>;

template<typename T, HalfAtomicArrayBuilderAllocator Allocator>
class HalfAtomicArrayBuilder {
public:
    static constexpr uint64_t OFFSET_MASK = ((uint64_t)1 << 32) - 1;

    struct alignas(std::max(alignof(uint32_t), alignof(T))) BlockHeader {
        Ptr32<BlockHeader> prev;
        uint32_t capacity;
    };

    struct EndSentinel { };
    struct BlockIterator {
        Allocator::CopyType allocator;
        Ptr32<BlockHeader> current;
        uint32_t capacity;

        BlockIterator& operator++() {
            Ptr32<BlockHeader> prev = current.get(allocator)->prev;
            allocator.free32(current.offset);

            current = prev;
            if (current) {
                capacity = current[allocator]->capacity;
            } else {
                capacity = 0;
            }

            return *this;
        }

        ArrayView<T> operator*() {
            return { (T*)allocator.address(current.offset + sizeof(BlockHeader)), capacity };
        }

        bool operator!=(EndSentinel) { return current != nullptr; }
    };

    HalfAtomicArrayBuilder(Allocator allocator)
        : allocator(allocator) { }

    void pushBack(T item) {
        while (true) {
            uint64_t pos = position.fetch_add(1, std::memory_order_acquire);
            Info i { pos };
            uint32_t newCapacity;
            if (i.storage) {
                BlockHeader* currentHeader = i.storage[allocator];
                if (i.offset < currentHeader->capacity) {
                    ((T*)allocator.address(i.storage.offset + sizeof(BlockHeader)))[i.offset] = item;
                    return;
                }
                newCapacity = 2 * currentHeader->capacity;
            } else {
                newCapacity = 4;
            }

            Ptr32<BlockHeader> newStorage { allocator.allocate32(alignof(BlockHeader), sizeof(BlockHeader) + newCapacity * sizeof(T)) };
            BlockHeader* newHeader = newStorage[allocator];
            newHeader->capacity = newCapacity;
            newHeader->prev = i.storage;

            Info j { .offset = 0, .storage = newStorage };
            bool success;
            do {
                success = position.compare_exchange_weak(pos, (uint64_t)j, std::memory_order_release);
            } while (Info { pos }.storage == i.storage);
            if (!success) {
                allocator.free32(newStorage.offset);
            }
        }
    }

    // must iterate through all blocks to deallocate them!
    // blocks are in reverse order
    auto takeUnsafe() {
        uint64_t pos = position.read(std::memory_order_relaxed);
        position.store(0, std::memory_order_relaxed);
        return iterate(pos);
    }

    void clear() {
        uint64_t pos = position.exchange(0, std::memory_order_acquire);
        for (auto _ : iterate(pos)) { }
    }

    ~HalfAtomicArrayBuilder() {
        uint64_t pos = position.read(std::memory_order_relaxed);
        for (auto _ : iterate(pos)) { }
    }

private:
    struct Info {
        Info(uint64_t pos)
            : offset(pos & OFFSET_MASK), storage { pos >> 32 } { }
        uint32_t offset;
        Ptr32<BlockHeader> storage;

        uint64_t pos() {
            return ((uint64_t)storage.offset << 32) | (uint64_t)offset;
        }
    };

    auto iterate(uint64_t pos) {
        Info i { pos };
        struct {
            BlockIterator it;
            BlockIterator begin() { return it; }
            EndSentinel end() { return {}; }
        } r { BlockIterator { allocator, i.storage, i.offset } };
        return r;
    }

    std::atomic<uint64_t> position = 0;

public:
    [[no_unique_address]] Allocator allocator;
};

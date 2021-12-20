#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/ArrayView.hpp"
#include "memory/TrailingArray.hpp"
#include "util/target.hpp"
#include "util/types.hpp"
#include <atomic>
#include <bit>
#include <mutex>

template<typename T, typename Allocator>
class HalfAtomicArrayBuilder : public AllocatorStorage<Allocator> {
public:
    static constexpr uint64_t OFFSET_MASK = ((uint64_t)1 << 32) - 1;

    struct BlockHeader : TrailingArray<T> {
        // for the last block next points to back to the first block
        Ptr32<BlockHeader> next;
        uint32_t capacity;
    };

    struct EndSentinel { };
    struct BlockIterator {
        Allocator allocator;
        Allocator::Ptr<BlockHeader> current;

        BlockIterator& operator++() {
            Allocator::Ptr<BlockHeader> next = allocator[current]->next;
            allocator.free(current);
            current = next;
            return *this;
        }

        ArrayView<T> operator*() {
            BlockHeader* header = allocator[current];
            return { header->array(), header->capacity };
        }

        bool operator!=(EndSentinel) { return current != nullptr; }
    };

    using AllocatorStorage<Allocator>::AllocatorStorage;

    void pushBack(T item, Allocator::Parameter alloc = {}) {
        Allocator allocator = getAllocator(alloc);
        while (true) {
            uint64_t pos = position.fetch_add(1, std::memory_order_acquire);
            Info info { pos };
            uint32_t newCapacity;
            BlockHeader* currentHeader;
            if (info.storage) {
                currentHeader = allocator[info.storage];
                if (info.offset < currentHeader->capacity) {
                    currentHeader->array()[info.offset] = item;
                    return;
                }
                newCapacity = 2 * currentHeader->capacity;
            } else {
                currentHeader = nullptr;
                newCapacity = 4;
            }

            Ptr32<BlockHeader> newStorage = BlockHeader::allocate(newCapacity, allocator);
            BlockHeader* newHeader = allocator[newStorage];
            newHeader->capacity = newCapacity;
            if (currentHeader) {
                // This information might be out of date, but it doesn't matter
                // since in that case we are guaranteed ot fail.
                newHeader->next = currentHeader->next;
            } else
                newHeader->next = newHeader;

            Info newInfo { .offset = 0, .storage = newStorage };
            bool success;
            do {
                success = position.compare_exchange_weak(pos, (uint64_t)newInfo, std::memory_order_release);
            } while (Info { pos }.storage == info.storage);
            if (success && currentHeader)
                currentHeader->next = newHeader;
            if (!success)
                allocator.free(newStorage);
        }
    }

    // must iterate through all blocks to deallocate them
    auto takeUnsafe(Allocator::Parameter alloc = {}) {
        Allocator allocator = getAllocator(alloc);
        uint64_t pos = position.read(std::memory_order_relaxed);
        position.store(0, std::memory_order_relaxed);

        Info info { pos };
        Ptr32<BlockHeader> first = nullptr;
        if (info.storage) {
            BlockHeader* last = allocator[info.storage];
            first = last->next;
            last->next = nullptr;
            last->capacity = info.offset;
        };
        struct {
            BlockIterator it;
            BlockIterator begin() { return it; }
            EndSentinel end() { return {}; }
        } r { BlockIterator { allocator, info.storage } };
        return r;
    }

    void clearUnsafe(Allocator::Parameter alloc = {}) {
        for (auto _ : takeUnsafe(pos, alloc)) { }
    }

    ~HalfAtomicArrayBuilder(Allocator::Parameter alloc = {}) {
        clearUnsafe(alloc);
    }

private:
    struct Info {
        Info(uint64_t pos)
            : offset(pos & OFFSET_MASK), storage((Allocator::Ptr<BlockHeader>)(Allocator::uintptr_t)(pos >> 32)) {
            static_assert(sizeof(Allocator::uintptr_t) <= sizeof(uint32_t));
        }
        uint32_t offset;
        Allocator::Ptr<BlockHeader> storage;

        uint64_t pos() {
            return ((uint64_t)(Allocator::uintptr_t)storage << 32) | (uint64_t)offset;
        }
    };

    std::atomic<uint64_t> position = 0;
};

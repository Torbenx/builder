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
    struct BlockHeader : TrailingArray<T> {
        // for the last block next points to back to the first block
        Allocator::Ptr<BlockHeader> next;
        uint_t capacity;
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
            storage_t pos = position.fetch_add(1, std::memory_order_acquire);
            Info info { pos };
            uint_t newCapacity;
            BlockHeader* currentHeader;
            if (info.storage) {
                currentHeader = allocator[info.storage];
                if (info.offset < currentHeader->capacity) {
                    currentHeader->array()[info.offset] = item;
                    return;
                }
                newCapacity = std::min(2 * currentHeader->capacity, MAX_CAPACITY);
            } else {
                currentHeader = nullptr;
                newCapacity = 4;
            }

            Allocator::Ptr<BlockHeader> newStorage = BlockHeader::allocate(newCapacity, allocator);
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
                success = position.compare_exchange_weak(pos, (storage_t)newInfo, std::memory_order_release);
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
        storage_t pos = position.read(std::memory_order_relaxed);
        position.store(0, std::memory_order_relaxed);

        Info info { pos };
        Allocator::Ptr<BlockHeader> first = nullptr;
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
    template<typename T>
    struct StorageTraits;
    template<>
    struct StorageTraits<uint32_t> {
        using type = uint64_t;
    };
    template<>
    struct StorageTraits<uint16_t> {
        using type = uint32_t;
    };
    template<>
    struct StorageTraits<uint8_t> {
        using type = uint16_t;
    };
    using uint_t = Allocator::uintptr_t;
    using storage_t = typename StorageTraits<uint_t>::type;
    static constexpr int STORAGE_SHIFT = sizeof(uint_t) * 8;
    static constexpr storage_t OFFSET_MASK = ((storage_t)1 << STORAGE_SHIFT) - 1;
    static constexpr uint_t MAX_CAPACITY = OFFSET_MASK / 4;

    struct Info {
        Info(storage_t pos)
            : offset(pos & OFFSET_MASK), storage((Allocator::Ptr<BlockHeader>)(uint_t)(pos >> STORAGE_SHIFT)) { }
        uint_t offset;
        Allocator::Ptr<BlockHeader> storage;

        storage_t pos() {
            return ((storage_t)(uint_t)storage << STORAGE_SHIFT) | (storage_t)offset;
        }
    };

    std::atomic<storage_t> position = 0;
};

#pragma once

#include "containers/ArrayView.hpp"
#include "memory/Allocator.hpp"
#include "memory/TrailingArray.hpp"
#include "util/target.hpp"
#include "util/types.hpp"
#include <atomic>
#include <bit>
#include <mutex>

template<typename T, typename Allocator>
class HalfAtomicArrayBuilder : public AllocatorStorage<Allocator> {
private:
    template<typename S>
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

    template<typename S = void>
    using Ptr = typename Allocator::template Ptr<S>;
    using uint_t = typename Allocator::uintptr_t;
    using storage_t = typename StorageTraits<uint_t>::type;

public:
    struct BlockHeader : TrailingArray<BlockHeader, T> {
        // for the last block next points to back to the first block
        Ptr<BlockHeader> next;
        uint_t size = 0;
        uint_t capacity = 0;
    };

    struct EndSentinel { };
    struct BlockIterator {
        Allocator allocator;
        Ptr<BlockHeader> current;

        BlockIterator& operator++() {
            BlockHeader* cur = allocator[current];
            Ptr<BlockHeader> next = cur->next;
            BlockHeader::free(current, cur->capacity, allocator);
            current = next;
            return *this;
        }

        ArrayView<T> operator*() {
            BlockHeader* header = allocator[current];
            return { header->array(), header->size };
        }

        bool operator!=(EndSentinel) { return current != nullptr; }
    };

    using AllocatorStorage<Allocator>::AllocatorStorage;

    void pushBack(const T& item, typename Allocator::Parameter alloc = {}) {
        Allocator allocator = this->getAllocator(alloc);
        while (true) {
            storage_t pos = position.fetch_add(1, std::memory_order_acquire);
            Info info { pos };
            uint_t newCapacity;
            BlockHeader* currentHeader;
            if (info.storage) {
                currentHeader = allocator[info.storage];
                if (info.offset < currentHeader->capacity) {
                    std::construct_at(currentHeader->array() + info.offset, item);
                    return;
                }
                newCapacity = std::min<uint_t>(2 * currentHeader->capacity, MAX_CAPACITY);
            } else {
                currentHeader = nullptr;
                newCapacity = 4;
            }

            Ptr<BlockHeader> newStorage = BlockHeader::allocate(newCapacity, allocator);
            BlockHeader* newHeader = allocator[newStorage];
            newHeader->size = newCapacity;
            newHeader->capacity = newCapacity;
            if (currentHeader) {
                // This information might be out of date, but it doesn't matter
                // since in that case we are guaranteed ot fail.
                newHeader->next = currentHeader->next;
            } else
                newHeader->next = newStorage;

            Info newInfo = Info::freshStorage(newStorage);
            bool success;
            do {
                success = position.compare_exchange_weak(pos, newInfo.pos(), std::memory_order_release);
            } while (Info { pos }.storage == info.storage);
            if (success && currentHeader)
                currentHeader->next = newStorage;
            if (!success)
                BlockHeader::free(newStorage, newCapacity, allocator);
        }
    }

    // caller must iterate through all blocks to deallocate them
    auto takeUnsafe(typename Allocator::Parameter alloc = {}) {
        Allocator allocator = this->getAllocator(alloc);
        storage_t pos = position.load(std::memory_order_relaxed);
        position.store(0, std::memory_order_relaxed);

        Info info { pos };
        Ptr<BlockHeader> first = nullptr;
        if (info.storage) {
            BlockHeader* last = allocator[info.storage];
            first = last->next;
            last->next = nullptr;
            last->size = info.offset;
        };
        struct {
            BlockIterator it;
            BlockIterator begin() { return it; }
            EndSentinel end() { return {}; }
        } r { BlockIterator { allocator, first } };
        return r;
    }

    void clearUnsafe(typename Allocator::Parameter alloc = {}) {
        for ([[maybe_unused]] auto _ : takeUnsafe(alloc)) { }
    }

    ~HalfAtomicArrayBuilder(typename Allocator::Parameter alloc = {}) {
        clearUnsafe(alloc);
    }

private:
    static constexpr int STORAGE_SHIFT = sizeof(uint_t) * 8;
    static constexpr storage_t OFFSET_MASK = ((storage_t)1 << STORAGE_SHIFT) - 1;
    static constexpr uint_t MAX_CAPACITY = OFFSET_MASK / 4;

    struct Info {
        Info(storage_t pos)
            : offset(pos & OFFSET_MASK), storage((Ptr<BlockHeader>)(uint_t)(pos >> STORAGE_SHIFT)) { }
        Info(uint_t offset, Ptr<BlockHeader> storage)
            : offset(offset), storage(storage) { }
        uint_t offset;
        Ptr<BlockHeader> storage;

        storage_t pos() {
            return ((storage_t)storage.addr() << STORAGE_SHIFT) | (storage_t)offset;
        }

        static Info freshStorage(Ptr<BlockHeader> storage) { return { 0, storage }; }
    };

    std::atomic<storage_t> position = 0;
};

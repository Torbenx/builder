#pragma once

#include "util/types.hpp"
#include <atomic>


namespace Tag {
struct Memory {
    static constexpr char tag[] = "memory";
    static constexpr bool verifyEnabled = true;
};
};

namespace mem {

constexpr size_t BLOCK_SIZE = 1024;

struct alignas(BLOCK_SIZE) SubAllocatedBlock {
    using mask_t = uint64_t;
    static constexpr size_t SLOT_SIZE = 16;
    static constexpr size_t SLOT_COUNT = sizeof(mask_t) * CHAR_BIT;
    static_assert(SLOT_COUNT * SLOT_SIZE == BLOCK_SIZE);

    struct AllocationLink {
        AllocationLink(size_t size)
            : size(size) { }
        AllocationLink(AllocationLink&&) = delete;
        AllocationLink* next = nullptr;
        size_t size = 0;
        void* allocation() { return this; }
    };
    static_assert(sizeof(AllocationLink) == SLOT_SIZE);

    struct Owner {
        std::atomic<AllocationLink*> first = nullptr;
    };

    struct Control {
        static constexpr mask_t bit(int pos) { return (mask_t)1 << pos; }
        static constexpr mask_t bitsBelow(int pos) { return bit(pos) - 1; }

        std::atomic<mask_t> freeMask = ~bit(SLOT_SIZE - 1);
        Owner* owner = nullptr;

        SubAllocatedBlock* block() { return (SubAllocatedBlock*)((std::byte*)this - sizeof(SubAllocatedBlock::storage)); }

        void reserve(int begin, int size) {
            mask_t selMask = bitsBelow(size) << begin;
            freeMask &= ~selMask;
        }
        void free(int begin, int size) {
            mask_t selMask = bitsBelow(size) << begin;
            freeMask |= selMask;
        }
    };
    static_assert(sizeof(Control) == SLOT_SIZE);

    static SubAllocatedBlock* block(void* address) {
        uintptr_t addr = (uintptr_t)address;
        addr &= ~(size_t)(alignof(SubAllocatedBlock) - 1);
        return (SubAllocatedBlock*)addr;
    }

    static void do_some_deleting(Owner* currentOwner, void* addr, size_t size) {
        auto* block = SubAllocatedBlock::block(addr);
        Owner* owner = block->control.owner;
        if (owner == currentOwner) {

        } else {
            AllocationLink* link = new (addr) AllocationLink(size);
            AllocationLink* first = owner->first.load(std::memory_order_relaxed);
            do {
                link->next = first;
            } while (!owner->first.compare_exchange_weak(first, link, std::memory_order_seq_cst));
        }
    }

    std::byte storage[BLOCK_SIZE - sizeof(Control)];
    Control control;
};
static_assert(sizeof(SubAllocatedBlock) == BLOCK_SIZE);
static_assert(alignof(SubAllocatedBlock) == BLOCK_SIZE);

}
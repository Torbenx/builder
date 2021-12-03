#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/AtomicBumpAllocator.hpp"
#include "containers/IdAllocator.hpp"
#include "containers/RangeAllocator.hpp"

namespace Tag {
struct Task {
    static constexpr char tag[] = "task";
    static constexpr bool verifyEnabled = true;
};
}

namespace task {
   
using utick_t = uint32_t;

class Heap {
public:
    static constexpr size_t TEMPORARY_ALLOCATION_DEFAULT_ALIGNMENT = 16;
    struct alignas(TEMPORARY_ALLOCATION_DEFAULT_ALIGNMENT) TemporaryAllocationHeader {
        struct MyCallback {
            uint32_t operator()(uint32_t capacity) {
                void* addr = addressToTickBase(this) + TEMPORARY_REGION_OFFSET + capacity;
                platform::commitMemory(addr, platform::pageSize());
                return capacity + platform::pageSize();
            }
        };

        AtomicBumpAllocator<MyCallback, TEMPORARY_ALLOCATION_DEFAULT_ALIGNMENT> allocator;

        TemporaryAllocationHeader(uint32_t initialCapacity)
            : allocator { initialCapacity, sizeof(TemporaryAllocationHeader), {} } {};
    };
    class TemporaryAllocator {
    public:
        uint32_t allocate32(int_t alignment, int_t size) {
            return TEMPORARY_REGION_OFFSET + header()->allocator.allocate(alignment, size);
        }
        void free32(uint32_t) { }
        void* address(uint32_t offset) { return tickBase + offset; }
        uint32_t offset(void* addr) { return (std::byte*)addr - tickBase; }

    private:
        std::byte* tickBase;
        TemporaryAllocationHeader* header() { return (TemporaryAllocationHeader*)(tickBase + TEMPORARY_REGION_OFFSET); }
        TemporaryAllocator(std::byte* tickBase)
            : tickBase(tickBase) { }
        friend class Heap;
    };

    class PersistentAllocator {
    public:
        uint32_t allocate32(int_t alignment, int_t size) {
            return PERSISTENT_REGION_OFFSET + allocator.allocate(alignment, size);
        }
        void free32(uint32_t) { VERIFY_NOT_REACHED(Task); }
        void* address(uint32_t offset) { return tickBase + offset; }
        uint32_t offset(void* addr) { return (std::byte*)addr - tickBase; }

    private:
        std::byte* tickBase;
        RangeAllocator& allocator;
        PersistentAllocator(std::byte* tickBase, RangeAllocator& allocator)
            : tickBase(tickBase), allocator(allocator) { }
        friend class Heap;
    };

    class TickHelper {
    public:
        PersistentAllocator persistentAllocator() { return { tickBase, heap->m_persistentAllocator }; }
        TemporaryAllocator temporaryAllocator() { return { tickBase }; }
        void* address(uint32_t offset) { return tickBase + offset; }
        uint32_t offset(void* addr) { return (std::byte*)addr - tickBase; }

    private:
        TickHelper(Heap* heap, std::byte* tickBase)
            : heap(heap), tickBase(tickBase) { }
        Heap* heap = nullptr;
        std::byte* tickBase = nullptr;
        friend class Heap;
    };

    class Helper {
    public:
        TickHelper tick(utick_t tick) { return { heap, tickBase(base, tick) }; }
        Helper() { }

    private:
        Heap* heap = nullptr;
        std::byte* base = nullptr;
        Helper(Heap* heap, std::byte* base)
            : heap(heap), base(base) { }
        friend class Heap;
    };

    static utick_t addressToUTick(void* addr) { return ((uintptr_t)addr >> HEAP_BITS) & TICK_MASK; }
    static std::byte* addressToBase(void* addr) { return (std::byte*)((uintptr_t)addr & ADDR_BASE_MASK); }
    static std::byte* addressToTickBase(void* addr) { return (std::byte*)((uintptr_t)addr & ~ADDR_HEAP_MASK); }
    static std::byte* heapToBase(void* heap) { return (std::byte*)heap - HEAP_REGION_SIZE; }
    static Heap* addressToHeap(void* addr) { return (Heap*)(addressToBase(addr) + HEAP_REGION_SIZE); }
    static TickHelper addressToTickHelper(void* addr) { return { addressToHeap(addr), addressToTickBase(addr) }; }
    static Helper addressToHelper(void* addr) { return { addressToHeap(addr), addressToBase(addr) }; }

    void* operator new(size_t size) {
        VERIFY(Task, size == sizeof(Heap));
        size_t extraSpace = alignmentCeil(platform::pageSize(), sizeof(Heap));
        std::byte* base = (std::byte*)platform::reserveMemoryRegion(memoryRegionSize(), memoryRegionAlignment());
        void* heap = base + HEAP_REGION_SIZE;
        platform::commitMemory(heap, extraSpace);
        return heap;
    }

    void operator delete(void* heap) {
        platform::releaseMemoryRegion(heapToBase(heap), memoryRegionSize());
    }

    static constexpr auto PERSISTENT_ALLOCATOR_GETTER = [](void* addr) -> PersistentAllocator { return addressToTickHelper(addr).persistentAllocator(); };
    static constexpr auto TEMPORARY_ALLOCATOR_GETTER = [](void* addr) -> TemporaryAllocator { return addressToTickHelper(addr).temporaryAllocator(); };
    using EmptyInplacePersistentAllocator = Allocator32BitWrapper<PERSISTENT_ALLOCATOR_GETTER>;
    using EmptyInplaceTemporaryAllocator = Allocator32BitWrapper<TEMPORARY_ALLOCATOR_GETTER>;

    Heap() {
        for (int_t i = 0; i < (int_t)MAX_TICKS; i++) {
            initializeTick(i);
        }
    }

    std::byte* base() { return heapToBase(this); }
    TickHelper tick(utick_t tick) { return { this, tickBase(tick) }; }
    Helper helper() { return { this, base() }; }

private:
    static constexpr size_t TICK_BITS = 4;
    static constexpr size_t HEAP_BITS = 24; // 16 MiB

public:
    static constexpr size_t MAX_TICKS = (size_t)1 << TICK_BITS;

private:
    static constexpr size_t MAX_HEAP_SIZE = (size_t)1 << HEAP_BITS;
    static constexpr size_t TICK_MASK = MAX_TICKS - 1;
    static constexpr uintptr_t ADDR_BASE_MASK = ~(MAX_TICKS * MAX_HEAP_SIZE - 1);
    static constexpr uintptr_t ADDR_HEAP_MASK = MAX_HEAP_SIZE - 1;

    static constexpr size_t HEAP_REGION_SIZE = MAX_TICKS * MAX_HEAP_SIZE;
    static size_t memoryRegionAlignment() { return HEAP_REGION_SIZE; }
    static size_t memoryRegionSize() { return HEAP_REGION_SIZE * 2; }

    static constexpr size_t NULL_REGION_SIZE = 4096 * 8;
    static constexpr size_t TEMPORARY_REGION_OFFSET = NULL_REGION_SIZE;
    static constexpr size_t PERSISTENT_REGION_OFFSET = MAX_HEAP_SIZE / 2;

    static constexpr size_t TEMPORARY_REGION_SIZE = PERSISTENT_REGION_OFFSET - TEMPORARY_REGION_OFFSET;
    static constexpr size_t PERSISTENT_REGION_SIZE = MAX_HEAP_SIZE - PERSISTENT_REGION_OFFSET;

    static std::byte* tickBase(std::byte* base, utick_t tick) { return base + (tick & TICK_MASK) * MAX_HEAP_SIZE; }

    RangeAllocator m_persistentAllocator { PERSISTENT_REGION_SIZE };

    std::byte* tickBase(utick_t tick) { return tickBase(base(), tick); }

    void initializeTick(utick_t tick) {
        platform::commitMemory(tickBase(tick), MAX_HEAP_SIZE);

        std::byte* temporaryBegin = tickBase(tick) + TEMPORARY_REGION_OFFSET;
        //platform::commitMemory(temporaryBegin, platform::pageSize());
        new (temporaryBegin) TemporaryAllocationHeader(platform::pageSize());
    }
};

}

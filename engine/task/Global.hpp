#pragma once

#include "containers/Allocator32Bit.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include "containers/AtomicBumpAllocator.hpp"
#include "containers/IdAllocator.hpp"
#include "containers/RangeAllocator.hpp"
#include "task/Scheduler.hpp"
#include "util/coroutines.hpp"
#include <list>

namespace task {

namespace detail {
    inline int_t tick = -1;
}

class Global {
public:
    struct Dependency {
        TaskHandle task;
        int16_t ticksAhead;
        int16_t refCount;

        bool operator==(const Dependency& other) const {
            return task == other.task && ticksAhead == other.ticksAhead;
        }
    };
    struct DependencyQueueItem {
        TaskHandle task;
        int16_t ticksAhead;
        int16_t refCountDelta;

        Dependency asEmptyDependency() {
            return { task, ticksAhead, 0 };
        }
    };

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
        friend class Global;
    };

    class InstanceAllocator {
    public:
        uint32_t allocate32(int_t alignment, int_t size) {
            return INSTANCE_REGION_OFFSET + allocator.allocate(alignment, size);
        }
        void free32(uint32_t);
        void* address(uint32_t offset) { return tickBase + offset; }
        uint32_t offset(void* addr) { return (std::byte*)addr - tickBase; }

    private:
        std::byte* tickBase;
        RangeAllocator& allocator;
        InstanceAllocator(std::byte* tickBase, RangeAllocator& allocator)
            : tickBase(tickBase), allocator(allocator) { }
        friend class Global;
    };

    class TickHelper {
    public:
        InstanceAllocator instanceAllocator() { return { tickBase, global->m_instanceAllocator }; }
        TemporaryAllocator temporaryAllocator() { return { tickBase }; }
        void* address(uint32_t offset) { return tickBase + offset; }
        uint32_t offset(void* addr) { return (std::byte*)addr - tickBase; }

    private:
        Global* global;
        std::byte* tickBase;
        TickHelper(Global* global, int_t tick)
            : global(global), tickBase(global->tickBase(tick)) { }
        friend class Global;
    };

    static int_t addressToTick(void* addr) { return ((uintptr_t)addr >> HEAP_BITS) & TICK_MASK; }
    static std::byte* addressToBase(void* addr) { return (std::byte*)((uintptr_t)addr & ADDR_BASE_MASK); }
    static std::byte* addressToTickBase(void* addr) { return (std::byte*)((uintptr_t)addr & ~ADDR_HEAP_MASK); }

    Global() {
        base = (std::byte*)platform::reserveMemoryRegion(MAX_HEAP_SIZE * MAX_TICKS, MAX_HEAP_SIZE * MAX_TICKS);
        for (int_t i = 0; i < (int_t)MAX_TICKS; i++) {
            initializeTick(i);
        }
    }

    ~Global() {
        platform::releaseMemoryRegion(base, MAX_HEAP_SIZE * MAX_TICKS);
    }

    TaskHandle newTask(stdx::coroutine_handle<> coro, int_t tick) {
        TaskHandle task = { taskIdAllocator.allocateId() };
        coroutines[task.id] = coro;
        return task;
    }

    InstanceAllocator instanceAllocator(int_t tick) { return { tickBase(tick), m_instanceAllocator }; }
    TemporaryAllocator temporaryAllocator(int_t tick) { return { tickBase(tick) }; }
    TickHelper tick(int_t tick) { return { this, tick }; }

    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(TaskHandle task) { return dependencyQueues[task.id]; }

    std::atomic<int16_t>& signal(TaskHandle task, int_t tick) { return ((std::atomic<int16_t>*)(tickBase(tick) + SIGNAL_REGION_OFFSET))[task.id]; }
    std::atomic<int16_t>& signalDelta(TaskHandle task, int_t tick) { return ((std::atomic<int16_t>*)(tickBase(tick) + SIGNAL_DELTA_REGION_OFFSET))[task.id]; }

    void queue(TaskHandle task, int16_t ticksAhead) {
        if (detail::tick + ticksAhead >= (int_t)MAX_TICKS)
            return;
        queues[detail::tick + ticksAhead].push_back(coroutines[task.id]);
    }

    void enter() {
    loop:
        for (int_t i = 0; i < (int_t)MAX_TICKS; i++) {
            if (queues[i].size() > 0) {
                auto coro = queues[i].front();
                queues[i].pop_front();
                detail::tick = i;
                coro.resume();
                goto loop;
            }
        }
    }

private:
    static constexpr size_t TICK_BITS = 4;
    static constexpr size_t HEAP_BITS = 24; // 16 MiB
    static constexpr size_t MAX_TICKS = (size_t)1 << TICK_BITS;
    static constexpr size_t MAX_HEAP_SIZE = (size_t)1 << HEAP_BITS;
    static constexpr size_t TICK_MASK = MAX_TICKS - 1;
    static constexpr uintptr_t ADDR_BASE_MASK = ~(MAX_TICKS * MAX_HEAP_SIZE - 1);
    static constexpr uintptr_t ADDR_HEAP_MASK = MAX_HEAP_SIZE - 1;

    static constexpr size_t NULL_REGION_SIZE = 4096 * 8;
    static constexpr size_t SIGNAL_REGION_SIZE = NULL_REGION_SIZE;
    static constexpr size_t SIGNAL_DELTA_REGION_SIZE = SIGNAL_REGION_SIZE;
    static constexpr size_t INSTANCE_REGION_SIZE = 8 * 1024 * 1024;

    static constexpr size_t MAX_TASKS = SIGNAL_REGION_SIZE / sizeof(int16_t);

    static constexpr size_t SIGNAL_REGION_OFFSET = NULL_REGION_SIZE;
    static constexpr size_t SIGNAL_DELTA_REGION_OFFSET = SIGNAL_REGION_OFFSET + SIGNAL_REGION_SIZE;
    static constexpr size_t INSTANCE_REGION_OFFSET = SIGNAL_DELTA_REGION_OFFSET + SIGNAL_DELTA_REGION_SIZE;
    static constexpr size_t TEMPORARY_REGION_OFFSET = INSTANCE_REGION_OFFSET + INSTANCE_REGION_SIZE;

    static constexpr size_t TEMPORARY_REGION_SIZE = MAX_HEAP_SIZE - TEMPORARY_REGION_OFFSET;

    std::byte* base;

    RangeAllocator m_instanceAllocator { INSTANCE_REGION_SIZE };
    IdAllocator<> taskIdAllocator;

    stdx::coroutine_handle<> coroutines[MAX_TASKS];
    AtomicArrayBuilder<DependencyQueueItem> dependencyQueues[MAX_TASKS];
    std::list<stdx::coroutine_handle<>> queues[MAX_TICKS];

    std::byte* tickBase(int_t tick) { return base + (tick & TICK_MASK) * MAX_HEAP_SIZE; }

    void initializeTick(int_t tick) {
        platform::commitMemory(tickBase(tick), MAX_HEAP_SIZE);

        std::byte* temporaryBegin = tickBase(tick) + TEMPORARY_REGION_OFFSET;
        //platform::commitMemory(temporaryBegin, platform::pageSize());
        new (temporaryBegin) TemporaryAllocationHeader(platform::pageSize());

    }
};

namespace detail {
    inline thread_local Global* global = nullptr;
    inline thread_local TaskHandle currentTask = nullptr;
}

}

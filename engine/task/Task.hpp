#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/IdAllocator.hpp"
#include "containers/RangeAllocator.hpp"
#include "containers/Allocator32Bit.hpp"
#include "task/Description.hpp"
#include "util/coroutines.hpp"
#include <list>

namespace task {

template<Description desc>
class Promise;

template<Description desc = Description {}>
class Task {
public:
    using promise_type = Promise<desc>;

    ~Task() {
        VERIFY(Task, ptr == nullptr);
    }

    Task(Promise<desc>* ptr)
        : ptr(ptr) { }

    Promise<desc>* ptr;

    Promise<desc>* take() {
        Promise<desc>* r = ptr;
        ptr = nullptr;
        return r;
    }

    template<Description d>
    friend class Promise;
};

struct TaskHandle {
    uint32_t id = 0;

    bool operator==(const TaskHandle& other) const { return id == other.id; }
    bool operator==(std::nullptr_t) const { return id == 0; }

    TaskHandle& operator=(std::nullptr_t) {
        id = 0;
        return *this;
    }
    TaskHandle(std::nullptr_t) { }
    TaskHandle() { }
    TaskHandle(uint32_t id)
        : id(id) { }

    explicit operator bool() { return id != 0; }
};

class GlobalAllocator;

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

    Global() {
        allocate(16, 16);
    }

    TaskHandle newTask(stdx::coroutine_handle<> coro) {
        TaskHandle task = { taskIdAllocator.allocateId() };
        coroutines[task.id] = coro;
        return task;
    }

    Ptr32<> allocate(size_t alignment, size_t size) {
        return { heapAllocator.allocate(alignment, size) };
    }

    std::byte* heap(int_t tick) { return data(tick).heap; }
    GlobalAllocator allocator(int_t tick);

    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(TaskHandle task) { return dependencyQueues[task.id]; }
    stdx::coroutine_handle<>& coroutine(TaskHandle task) { return coroutines[task.id]; }
    std::atomic<int16_t>& signal(int_t tick, TaskHandle task) { return data(tick).signals[task.id]; }
    std::atomic<int16_t>& signalDelta(int_t tick, TaskHandle task) { return data(tick).signalDeltas[task.id]; }

    void queue(stdx::coroutine_handle<> handle, int_t tick) {
        if (tick >= MAX_TICKS)
            return;
        data(tick).queue.push_back(handle);
    }

    void enter() {
    loop:
        for (int_t i = 0; i < MAX_TICKS; i++) {
            if (data(i).queue.size() > 0) {
                auto coro = data(i).queue.front();
                data(i).queue.pop_front();
                coro.resume();
                goto loop;
            }
        }
    }

private:
    static constexpr int_t MAX_TASKS = 100;
    static constexpr size_t MAX_TICKS = 16;
    static constexpr size_t HEAP_SIZE = 1024;

    struct TickData {
        std::atomic<int16_t> signals[MAX_TASKS];
        std::atomic<int16_t> signalDeltas[MAX_TASKS];
        alignas(16) std::byte heap[HEAP_SIZE];
        std::list<stdx::coroutine_handle<>> queue;
    };

    RangeAllocator heapAllocator { HEAP_SIZE };
    IdAllocator<> taskIdAllocator;

    stdx::coroutine_handle<> coroutines[MAX_TASKS];
    AtomicArrayBuilder<DependencyQueueItem> dependencyQueues[MAX_TASKS];
    TickData tickData[MAX_TICKS];

    TickData& data(int_t tick) {
        VERIFY(Default, tick < MAX_TICKS);
        return tickData[tick & (MAX_TICKS - 1)];
    }
};

namespace detail {
    inline thread_local Global* global = nullptr;
    inline thread_local TaskHandle currentTask = nullptr;
    inline thread_local int_t tick = -1;
}

class GlobalAllocator {
public:
    int_t tick;
    uint32_t allocate32(int_t size) { return detail::global->allocate(__STDCPP_DEFAULT_NEW_ALIGNMENT__, size).offset; }
    uint32_t allocate32(int_t alignment, int_t size) { return detail::global->allocate(alignment, size).offset; }
    void free32(uint32_t offset) { }
    void* address(uint32_t offset) { return detail::global->heap(tick) + offset; }
    bool ownsAddress(void* address);
    uint32_t offset(void* address);
};

inline GlobalAllocator Global::allocator(int_t tick) {
    return GlobalAllocator { tick };
}

}

#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "task/Description.hpp"
#include "util/coroutines.hpp"

namespace task {

namespace detail {
    inline thread_local std::byte* heap = nullptr;
}

template<Description desc>
class Promise;

template<Description desc = Description {}>
class Task {
public:
    using promise_type = Promise<desc>;

    ~Task() {
        VERIFY(Task, ptr == nullptr);
    }

private:
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

    TaskHandle& operator=(std::nullptr_t) { id = 0; }
    TaskHandle(std::nullptr_t) { }
    TaskHandle() { }

    explicit operator bool() { return id != 0; }
};

template<typename T = void>
struct Ptr {
    uint32_t offset = 0;
    T* get() { return (T*)(detail::heap + offset); }
};

struct Environment { };

struct Global {
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

    std::byte* heap(int_t tick);

    AtomicArrayBuilder<DependencyQueueItem>& dependencyQueue(TaskHandle);
    stdx::coroutine_handle<>& coroutine(TaskHandle);
    std::atomic<int16_t>& signal(int_t tick, TaskHandle);
    std::atomic<int16_t>& signalDelta(int_t tick, TaskHandle);

    virtual void queue(stdx::coroutine_handle<>, int_t tick);
};

namespace detail {
    inline thread_local Global* global = nullptr;
    inline thread_local TaskHandle currentTask = nullptr;
}

}

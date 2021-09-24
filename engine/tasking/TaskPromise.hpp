#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "tasking/Task.hpp"

class TaskDependency {
public:
    TaskDependency() { }
    TaskDependency(Task<> task, size_t initialRefCount = 1)
        : m_task(task), m_refCount(initialRefCount) { }

    Task<> task() const { return m_task; }
    size_t refCount() const { return m_refCount.load(std::memory_order_relaxed); }
    size_t ref(size_t count = 1) { return m_refCount.fetch_add(count, std::memory_order_relaxed); }
    size_t unref(size_t count = 1) { return m_refCount.fetch_sub(count, std::memory_order_relaxed); }

    bool operator==(const TaskDependency& other) const { return task() == other.task(); }

private:
    Task<> m_task;
    std::atomic<int32_t> m_refCount;
};

struct GetGlobalAllocator {
    MemoryAllocator32& operator()() { return task_detail::allocator(); }
};

template<>
class TaskPromiseBase<void> {
public:
    void* operator new(size_t size) {
        MemoryAllocator32& alloc = task_detail::allocator();
        void* addr = alloc.address(alloc.allocate32(alignof(TaskPromiseBase<void>), size));
        return addr;
    }

    void operator delete(void* addr) {
        MemoryAllocator32& alloc = task_detail::allocator();
        VERIFY(Tasking, alloc.ownsAddress(addr));
        alloc.free32(alloc.offset(addr));
    }

    template<typename... Args>
    explicit TaskPromiseBase(Args&... args) { }
    TaskPromiseBase(const TaskPromiseBase&) = delete;
    TaskPromiseBase(TaskPromiseBase&&) = delete;

    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Tasking);
        std::abort();
    }

protected:
    AtomicArrayBuilder<Task<>, Allocator32BitWrapper<GetGlobalAllocator>> insertions;
    FlatSet<TaskDependency> dependencies;
};

template<typename Interface>
class TaskPromiseBase : public TaskPromiseBase<void> {
public:
    Interface interface;

    using TaskPromiseBase<void>::TaskPromiseBase;
};

template<typename Interface>
struct TaskPromise : TaskPromise<Interface> {
    using TaskPromise<Interface>::TaskPromise;

    Task<Interface> task() { return { task_detail::allocator().offset(this) }; }

    await_value<TaskPromise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    Task<Interface> get_return_object() {
        VERIFY(Tasking, task_detail::allocator().ownsAddress(this));
        return task();
    }

    template<typename Inf>
    future_task<Inf> acquire_future(Task<Inf> task) {
        return { this->task(), task };
    }
    template<typename Inf>
    past_task<Inf> acquire_past(Task<Inf> task) {
        return { this->task(), task };
    }

    future_task<Interface> flip(past_task<> task) {
        VERIFY(Tasking, task.base == this->task());
        return { task.target, this->task() };
    }
    past_task<Interface> flip(future_task<> task) {
        VERIFY(Tasking, task.base == this->task());
        return { task.target, this->task() };
    }
};
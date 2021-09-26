#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "task/Task.hpp"

class TaskDependency {
public:
    TaskDependency() { }
    TaskDependency(TaskRef<> task, size_t initialRefCount = 1)
        : m_task(task), m_refCount(initialRefCount) { }

    TaskRef<> task() const { return m_task; }
    size_t refCount() const { return m_refCount.load(std::memory_order_relaxed); }
    size_t ref(size_t count = 1) { return m_refCount.fetch_add(count, std::memory_order_relaxed); }
    size_t unref(size_t count = 1) { return m_refCount.fetch_sub(count, std::memory_order_relaxed); }

    bool operator==(const TaskDependency& other) const { return task() == other.task(); }

private:
    TaskRef<> m_task;
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
        VERIFY(Task, alloc.ownsAddress(addr));
        alloc.free32(alloc.offset(addr));
    }

    template<typename... Args>
    explicit TaskPromiseBase(Args&... args) { }
    TaskPromiseBase(const TaskPromiseBase&) = delete;
    TaskPromiseBase(TaskPromiseBase&&) = delete;

    auto initial_suspend() { return stdx::suspend_always(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Task);
        std::abort();
    }

    int_t tick() { return m_tick; }

private:
    AtomicArrayBuilder<TaskRef<>, Allocator32BitWrapper<GetGlobalAllocator>> insertions;
    FlatSet<TaskDependency> dependencies;
    int_t m_tick;

    template<typename Inf>
    friend class TaskPromise;
    template<typename Inf>
    friend class TaskPromiseBase;
};

template<typename Interface>
class TaskPromiseBase : public TaskPromiseBase<void> {
public:
    Interface interface;

    using TaskPromiseBase<void>::TaskPromiseBase;
};

namespace task_detail {
template<typename T>
struct InterfaceOf;

template<typename Inf, typename... Args>
struct InterfaceOf<Inf (*)(Args...)> {
    using type = Inf;
};

template<typename T>
using InterfaceOf_t = InterfaceOf<T>::type;
}

template<typename Interface>
class TaskPromise : public TaskPromiseBase<Interface> {
public:
    using TaskPromiseBase<Interface>::TaskPromiseBase;

    TaskRef<Interface> task() { return { task_detail::allocator().offset(this) }; }

    await_value<TaskPromise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    Task<Interface> get_return_object() {
        VERIFY(Task, task_detail::allocator().ownsAddress(this));
        return { this };
    }

    template<auto func, typename... Args>
    future_task<task_detail::InterfaceOf_t<decltype(func)>> spawn_future(Args&&... args) {
        spawnInternal(func, std::forward<Args>(args)...);
    }

    future_task<Interface> flip(past_task<> task) {
        VERIFY(Task, task.base == this->task());
        return { task.target, this->task() };
    }
    past_task<Interface> flip(future_task<> task) {
        VERIFY(Task, task.base == this->task());
        return { task.target, this->task() };
    }

private:
    template<typename Inf, typename... ForwardArgs, typename... FuncArgs>
    void spawnInternal(Task<Inf> (*func)(FuncArgs...), ForwardArgs&&... args) {
        Task<Inf> task = func(std::forward<ForwardArgs>(args)...);
        TaskPromise<Inf>* promise = task.take();
        promise->m_tick = this->m_tick;
        stdx::coroutine_handle<TaskPromise<Inf>>::from_promise(*promise).resume();
    }
};
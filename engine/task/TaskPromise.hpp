#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "task/Task.hpp"

namespace task {

struct GetGlobalAllocator {
    MemoryAllocator32& operator()() { return detail::allocator(); }
};

template<description desc>
class Promise {
public:
    void* operator new(size_t size) {
        MemoryAllocator32& alloc = detail::allocator();
        void* addr = alloc.address(alloc.allocate32(alignof(Promise<desc>), size));
        return addr;
    }

    void operator delete(void* addr) {
        MemoryAllocator32& alloc = detail::allocator();
        VERIFY(Task, alloc.ownsAddress(addr));
        alloc.free32(alloc.offset(addr));
    }

    template<typename... Args>
    explicit Promise(Args&... args) { }

    Promise(const Promise&) = delete;
    Promise(Promise&&) = delete;

    auto initial_suspend() { return stdx::suspend_always(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Task);
        std::abort();
    }

    int_t tick() { return m_tick; }

    //TaskRef<Interface> task() { return { task_detail::allocator().offset(this) }; }

    await_value<Promise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) {
        this->m_tick++;
        return std::forward<T>(t);
    }

    Task<desc> get_return_object() {
        VERIFY(Task, detail::allocator().ownsAddress(this));
        return { this };
    }

private:
    int_t m_tick;
    /*template<typename Inf, typename... ForwardArgs, typename... FuncArgs>
    void spawnInternal(Task<Inf> (*func)(FuncArgs...), ForwardArgs&&... args) {
        Task<Inf> task = func(std::forward<ForwardArgs>(args)...);
        TaskPromise<Inf>* promise = task.take();
        promise->m_tick = this->m_tick;
        stdx::coroutine_handle<TaskPromise<Inf>>::from_promise(*promise).resume();
    }*/
};

}
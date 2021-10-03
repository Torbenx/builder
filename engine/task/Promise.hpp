#pragma once

#include "containers/FlatSet.hpp"
#include "task/Task.hpp"

namespace task {

template<typename T = void>
struct Ptr {
    uint32_t offset = 0;
};

struct Environment {};

struct Heap {
    std::byte* get(int_t tickCount);
};

struct Dependies { };

template<description desc>
class Promise {
public:
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

    await_value<Promise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) {
        return std::forward<T>(t);
    }

    Task<desc> get_return_object() { return { this }; }

private:
    int_t tick = 0;
    Environment* env = nullptr;
    Heap* heap = nullptr;
    Ptr<typename decltype(desc)::Instance> instance;
    Dependies dependies;

    template<description d, typename... ForwardArgs, typename... FuncArgs>
    void spawnInternal(Task<d> (*func)(FuncArgs...), ForwardArgs&&... args) {
        Task<d> task = func(std::forward<ForwardArgs>(args)...);
        Promise<d>* promise = task.take();

        promise->tick = this->tick;
        promise->env = this->env;
        promise->heap = this->heap;

        stdx::coroutine_handle<Promise<d>>::from_promise(*promise).resume();
    }
};

}
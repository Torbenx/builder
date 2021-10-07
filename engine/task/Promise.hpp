#pragma once

#include "containers/FlatSet.hpp"
#include "task/Interface.hpp"

namespace task {

class PromiseBase {
protected:
    int_t tick = 0;
    int16_t signalCount = 0;
    Environment* env = nullptr;
    Global* global = nullptr;

    FlatSet<Global::Dependency> dependencies;

    TaskHandle task();

    template<Description d, typename... ForwardArgs, typename... FuncArgs>
    void spawnInternal(Task<d> (*func)(FuncArgs...), ForwardArgs&&... args) {
        Task<d> task = func(std::forward<ForwardArgs>(args)...);
        Promise<d>* promise = task.take();

        promise->tick = this->tick;
        promise->env = this->env;
        promise->global = this->global;

        stdx::coroutine_handle<Promise<d>>::from_promise(*promise).resume();
    }

    void signalDependencies();
};

template<Description desc>
class Promise : public PromiseBase {
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
    Ptr<typename decltype(desc)::Instance> instance;
};

}

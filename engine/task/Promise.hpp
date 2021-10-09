#pragma once

#include "containers/FlatSet.hpp"
#include "task/Interface.hpp"

namespace task {

class AwaitNextTick;
class AwaitFirstTick;

template<Description desc>
class InstanceExtractor {
public:
    template<typename T>
    InterfaceRef<T> get(int16_t ticksAhead) {
        return InterfaceRef<T>::createFromCurrent(promise->task, promise->template ptr<T>(), ticksAhead);
    }

    InstanceExtractor(Promise<desc>* promise)
        : promise(promise) { }

private:
    Promise<desc>* promise;
};

class PromiseBase {
protected:
    int_t tick = 0;
    int16_t signalCount = 0;
    Global* global = nullptr;
    TaskHandle task;

    FlatSet<Global::Dependency> dependencies;

    void signalDependencies();
    void setGlobals() {
        detail::currentTask = task;
        detail::global = global;
        detail::tick = tick;
    }
    static void clearGlobals() {
        detail::currentTask = nullptr;
        detail::global = nullptr;
        detail::tick = -1;
    }

    friend class AwaitNextTick;
    friend class AwaitFirstTick;
    template<Description d>
    friend class InstanceExtractor;

public:
    template<Description d, typename... ForwardArgs, typename... FuncArgs>
    static InstanceExtractor<d> spawnInternal(Task<d> (*func)(FuncArgs...), Global* global, int_t tick, ForwardArgs&&... args, bool immediate) {
        Promise<d>* promise = func(std::forward<ForwardArgs>(args)...).take();

        promise->global = global;
        promise->tick = tick;

        auto coro = stdx::coroutine_handle<Promise<d>>::from_promise(*promise);
        TaskHandle task = global->newTask(coro, tick);
        promise->instance = make32<typename Promise<d>::Instance_t>(global->instanceAllocator(tick));
        promise->task = task;

        if (immediate)
            coro.resume();

        return { promise };
    }

    template<auto f, typename... Args>
    auto spawnImmediate(Args&&... args) {
        auto ret = spawnInternal(f, global, tick, std::forward<Args>(args)..., true);
        setGlobals();
        return ret;
    }

    template<auto f, typename... Args>
    auto spawnLazy(Args&&... args) {
        auto ret = spawnInternal(f, global, tick, std::forward<Args>(args)..., false);
        return ret;
    }
};

class AwaitNextTick {
public:
    PromiseBase& promise;

    bool await_ready() { return false; }
    void await_suspend(stdx::coroutine_handle<>) {
        PromiseBase::clearGlobals();
        promise.signalDependencies();
    }
    void await_resume() {
        promise.setGlobals();
    }
};

class AwaitFirstTick {
public:
    PromiseBase& promise;

    bool await_ready() { return false; }
    void await_suspend(stdx::coroutine_handle<>) { }
    void await_resume() {
        promise.setGlobals();
    }
};

template<Description desc>
class Promise : public PromiseBase {
public:
    using Instance_t = typename decltype(desc)::Instance;

    template<typename... Args>
    explicit Promise(Args&... args) { }

    Promise(const Promise&) = delete;
    Promise(Promise&&) = delete;

    AwaitFirstTick initial_suspend() { return { *this }; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    AwaitNextTick yield_value(int) { return { *this }; }
    await_value<Promise&> await_transform(get_self_t) { return { *this }; }
    Task<desc> get_return_object() { return { this }; }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Task);
        std::abort();
    }

    template<typename T>
    Ptr32<T> ptr() {
        return (instance->*&Instance_t::interfaces).template get<T>();
    }

    template<typename T>
    T& get() {
        return *(T*)(global->tickBase(tick) + ptr<T>().offset);
    }

private:
    Ptr32<Instance_t> instance;
    friend class PromiseBase;
};

}

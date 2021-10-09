#pragma once

#include "containers/FlatSet.hpp"
#include "task/Interface.hpp"
#include "task/Description.hpp"

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

class AwaitNextTick;
class AwaitFirstTick;

template<Description desc>
class InstanceExtractor {
public:
    template<typename T>
    Ref<T> get(int16_t ticksAhead) {
        return Ref<T>::createFromCurrent(promise->task, promise->template ptr<T>(), ticksAhead);
    }

    InstanceExtractor(Promise<desc>* promise)
        : promise(promise) { }

private:
    Promise<desc>* promise;
};

class PromiseBase {
protected:
    int16_t signalCount = 0;
    Global* global = nullptr;
    TaskHandle task;

    FlatSet<Global::Dependency> dependencies;

    void signalDependencies();
    void setGlobals() {
        detail::currentTask = task;
        detail::global = global;
    }
    static void clearGlobals() {
        detail::currentTask = nullptr;
        detail::global = nullptr;
    }

    friend class AwaitNextTick;
    friend class AwaitFirstTick;
    template<Description d>
    friend class InstanceExtractor;

public:
    template<Description d, typename... ForwardArgs, typename... FuncArgs>
    static InstanceExtractor<d> spawnInternal(Task<d> (*func)(FuncArgs...), Global* global, ForwardArgs&&... args, bool immediate) {
        Promise<d>* promise = func(std::forward<ForwardArgs>(args)...).take();

        promise->global = global;

        auto coro = stdx::coroutine_handle<Promise<d>>::from_promise(*promise);
        TaskHandle task = global->newTask(coro, detail::tick);
        promise->instance = make32<typename Promise<d>::Instance_t>(global->instanceAllocator(detail::tick));
        promise->task = task;

        if (immediate)
            coro.resume();

        return { promise };
    }

    template<auto f, typename... Args>
    auto spawnImmediate(Args&&... args) {
        auto ret = spawnInternal(f, global, std::forward<Args>(args)..., true);
        setGlobals();
        return ret;
    }

    template<auto f, typename... Args>
    auto spawnLazy(Args&&... args) {
        auto ret = spawnInternal(f, global, std::forward<Args>(args)..., false);
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

    Promise() { }
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
    Ptr32<T> ptr() { return (instance->*&Instance_t::interfaces).template get<T>(); }
    template<typename T>
    T& get() {
        return *ptr<T>()[global->tick(detail::tick)];
    }

private:
    friend class PromiseBase;
    Ptr32<Instance_t> instance;
};

}

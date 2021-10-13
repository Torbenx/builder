#pragma once

#include "containers/FlatSet.hpp"
#include "task/Description.hpp"
#include "task/Interface.hpp"

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

template<Description desc>
class InstanceExtractor {
public:
    template<typename T>
    Ref<T> get(int16_t ticksAhead) {
        return Ref<T>::createFromCurrent(promise->task, promise->template ptr<T>(), ticksAhead);
    }

    InstanceExtractor(Promise<desc>* promise)
        : promise(promise) { }

    void submit() {
        if (detail::scheduler->signalSelfInitial(promise->task, detail::tick)) {
            TaskHandle temp = detail::currentTask;
            detail::currentTask = promise->task;
            stdx::coroutine_handle<Promise<desc>>::from_promise(*promise).resume();
            detail::scheduler->signalDependencies(promise->task, detail::tick);
            detail::currentTask = temp;
        }
        promise = nullptr;
    }

private:
    Promise<desc>* promise;
};

class PromiseBase {
protected:
    TaskHandle task;

    template<Description d>
    friend class InstanceExtractor;

public:
    template<Description d, typename... ForwardArgs, typename... FuncArgs>
    static InstanceExtractor<d> spawnInternal(Task<d> (*func)(FuncArgs...), ForwardArgs&&... args) {
        Promise<d>* promise = func(std::forward<ForwardArgs>(args)...).take();

        auto coro = stdx::coroutine_handle<Promise<d>>::from_promise(*promise);
        promise->task = detail::scheduler->newTask(coro);
        promise->instance = make32<typename Promise<d>::Instance_t>(detail::scheduler->heap().persistentAllocator());

        return { promise };
    }

    template<auto f, typename... Args>
    auto spawn(Args&&... args) {
        auto ret = spawnInternal(f, std::forward<Args>(args)...);
        return ret;
    }
};

template<Description desc>
class Promise : public PromiseBase {
public:
    using Instance_t = typename decltype(desc)::Instance;

    Promise() { }
    Promise(const Promise&) = delete;
    Promise(Promise&&) = delete;

    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    stdx::suspend_always yield_value(int) { return {}; }
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
        return *ptr<T>()[detail::scheduler->heap()];
    }

private:
    friend class PromiseBase;
    Ptr32<Instance_t> instance;
};

}

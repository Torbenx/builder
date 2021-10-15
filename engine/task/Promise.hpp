#pragma once

#include "containers/FlatSet.hpp"
#include "task/Description.hpp"
#include "task/Interface.hpp"

namespace task {

template<Description desc>
class Promise;

template<auto func>
class Spawn;

namespace detail {
    inline stdx::coroutine_handle<> getNext() {
        auto pair = detail::scheduler->next();
        detail::currentTask = pair.first;
        detail::tick = pair.second;
        if (pair.first) {
            return stdx::coroutine_handle<>::from_address(detail::scheduler->payload(pair.first));
        } else {
            return stdx::noop_coroutine();
        }
    }
}

struct AwaitNextTick {
    bool await_ready() { return false; }

    stdx::coroutine_handle<> await_suspend(stdx::coroutine_handle<>) {
        detail::scheduler->signalDependencies(detail::currentTask, detail::tick);
        return detail::getNext();
    }

    void await_resume() { }
};

struct AwaitTaskExecute {
    bool await_ready() { return detail::scheduler->isReady(detail::currentTask, detail::tick); }

    stdx::coroutine_handle<> await_suspend(stdx::coroutine_handle<>) {
        return detail::getNext();
    }

    void await_resume() { }
};

template<Description desc = Description {}>
class Task {
public:
    using promise_type = Promise<desc>;

    Task(Promise<desc>* promise)
        : promise(promise) { }

    Promise<desc>* take() {
        auto* temp = promise;
        promise = nullptr;
        return temp;
    }

    ~Task() {
        VERIFY(Task, promise == nullptr);
    }

private:
    Promise<desc>* promise;
};

template<Description desc>
class Promise {
public:
    Promise() { }
    Promise(const Promise&) = delete;
    Promise(Promise&&) = delete;

    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    AwaitNextTick yield_value(int) { return {}; }
    await_value<Promise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }
    Task<desc> get_return_object() { return { this }; }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Task);
        std::abort();
    }


    template<typename T>
    Ptr32<T> ptr() { return interfaces.template get<T>(); }
    template<typename T>
    T& get() {
        return *ptr<T>()[detail::scheduler->heap(detail::currentTask.root(), detail::tick)];
    }

private:
    Ptr32<typename decltype(desc)::InterfaceTuple> interfaces;
    Ptr32<typename decltype(desc)::DataTuple> data;
    template<auto func>
    friend class Spawn;
};

template<Description desc, typename... Args, Task<desc> (*func)(Args...)>
class Spawn<func> {
public:
    Spawn() {
        task = detail::scheduler->newTask(detail::currentTask.root(), detail::tick);
        interfaces = make32<Interfaces_t>(detail::scheduler->heap(detail::currentTask.root(), detail::tick).persistentAllocator());
        data = make32<Data_t>(detail::scheduler->heap(detail::currentTask.root(), detail::tick).persistentAllocator());
    }

    template<typename T>
    Ref<T> get(Scheduler::tickdiff_t ticksAhead) {
        return Ref<T>::createFromCurrent(task, interfaces.template get<T>(), ticksAhead);
    }

    template<typename... CallArgs>
    AwaitTaskExecute execute(CallArgs&&... args) {
        detail::activeTask = { task, 0 };
        Promise<desc>* promise = func(std::forward<CallArgs>(args)...).take();
        detail::activeTask = { nullptr, 0 };
        promise->interfaces = interfaces;
        promise->data = data;
        detail::scheduler->payload(task) = stdx::coroutine_handle<Promise<desc>>::from_promise(*promise).address();
        detail::scheduler->signalSelfInitial(task, detail::tick);
        return {};
    }

private:
    using Interfaces_t = typename decltype(desc)::InterfaceTuple;
    using Data_t = typename decltype(desc)::DataTuple;

    TaskHandle task;
    Ptr32<Interfaces_t> interfaces;
    Ptr32<Data_t> data;
};

}

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
        if (pair.first) {
            return stdx::coroutine_handle<>::from_address(detail::scheduler->payload(pair.first));
        } else {
            return stdx::noop_coroutine();
        }
    }
}

struct AwaitNextTick {
    Scheduler::TaskInternals& taskInternals;
    TaskInfo& taskInfo;

    bool await_ready() { return false; }

    stdx::coroutine_handle<> await_suspend(stdx::coroutine_handle<>) {
        taskInternals.signalDependencies(*detail::scheduler, taskInfo.task, taskInfo.tick);
        taskInfo.tick += 1;
        return detail::getNext();
    }

    void await_resume() { }
};

struct AwaitTaskExecute {
    bool await_ready([[context]] TaskInfo info) { return detail::scheduler->isReady(info.task, info.tick); }

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
    const int_t taskInfoOffset;
    template<typename... Args>
    Promise(Args&... args, TaskInfo& ti)
        : taskInfoOffset((std::byte*)&ti - (std::byte*)this) { }

    Promise(const Promise&) = delete;
    Promise(Promise&&) = delete;

    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    AwaitNextTick yield_value(int) { return { taskInternals, taskInfo() }; }
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
    auto get() {
        if constexpr (tuple_has_type_v<T, Interfaces_t>) {
            return SelfRef<T, true> { interfaces.template get<T>() };
        } else {
            return SelfRef<T, false> { data.template get<T>() };
        }
    }

private:
    using Interfaces_t = typename decltype(desc)::InterfaceTuple;
    using Data_t = typename decltype(desc)::DataTuple;

    Ptr32<Interfaces_t> interfaces;
    Ptr32<Data_t> data;
    template<auto func>
    friend class Spawn;

    Scheduler::TaskInternals taskInternals;
    TaskInfo& taskInfo() { return *(TaskInfo*)((std::byte*)this + taskInfoOffset); }
};

template<Description desc, typename... Args, Task<desc> (*func)(Args...)>
class Spawn<func> {
public:
    Spawn([[context]] TaskInfo info) {
        task = detail::scheduler->newTask(info.task.root(), info.tick);
        interfaces = make32<Interfaces_t>(detail::scheduler->heap(task.root(), info.tick).persistentAllocator());
        data = make32<Data_t>(detail::scheduler->heap(task.root(), info.tick).persistentAllocator());
    }

    template<typename T>
    auto get(tickdiff_t delay, [[context]] TaskInfo info) {
        if constexpr (tuple_has_type_v<T, Interfaces_t>) {
            return InterfaceRef<T>::createFromCurrent(task, interfaces.template get<T>(), delay);
        } else {
            return DataRef<T>::createToCurrent(task, data.template get<T>(), -delay);
        }
    }

    template<typename... CallArgs>
    AwaitTaskExecute execute(CallArgs&&... args, [[context]] TaskInfo info) {
        detail::activeTask = { task, (tickdiff_t)info.tick };
        Promise<desc>* promise = func(std::forward<CallArgs>(args)..., TaskInfo { task, 0 }).take();
        detail::activeTask = { nullptr, 0 };
        promise->interfaces = interfaces;
        promise->data = data;
        detail::scheduler->payload(task) = stdx::coroutine_handle<Promise<desc>>::from_promise(*promise).address();
        detail::scheduler->signalSelfInitial(task, info.tick);
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

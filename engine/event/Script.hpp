#pragma once

#include "platform/thread_local_storage.hpp"
#include "util/coroutines.hpp"
#include "util/types.hpp"
#include <algorithm>
#include <vector>

namespace Tag {
struct Event {
    static constexpr char tag[] = "event";
    static constexpr bool verifyEnabled = true;
};
}

namespace evt {

struct ScriptPromise;

inline int_t tick() { return (uintptr_t)plt::getThreadLocalPtr<plt::TLS_CURRENT_TICK_INDEX>(); }
inline void setTick(int_t tick) {
    // VERIFY(Event, tick >= 0);
    plt::setThreadLocalPtr<plt::TLS_CURRENT_TICK_INDEX>((void*)(uintptr_t)tick);
}

template<typename T, typename Base>
concept CompatibleCoroutinePromise = std::is_base_of_v<Base, T> && alignof(T) == alignof(Base) && sizeof(T) == sizeof(Base) && std::is_standard_layout_v<T>;
struct Script {
    using promise_type = ScriptPromise;
    Script() = default;
    Script(stdx::coroutine_handle<ScriptPromise> handle)
        : handle(handle) { }
    template<CompatibleCoroutinePromise<ScriptPromise> Promise>
    Script(stdx::coroutine_handle<Promise> handle)
        : handle(stdx::coroutine_handle<ScriptPromise>::from_address(handle.address())) { }

    static Script fromAddress(void* address) {
        return stdx::coroutine_handle<ScriptPromise>::from_address(address);
    }

    stdx::coroutine_handle<ScriptPromise> handle = {};

    void* address() const { return handle.address(); }
    ScriptPromise* promise() const { return &handle.promise(); }
    ScriptPromise* operator->() const { return promise(); }
    ScriptPromise& operator*() const { return *promise(); }
    explicit operator bool() const { return (bool)handle; }
    operator stdx::coroutine_handle<>() const { return handle; }
    bool operator==(const Script&) const = default;
    void deallocate() {
        if (handle) {
            handle.destroy();
            handle = {};
        }
    }
};

struct Executor {
    std::vector<stdx::coroutine_handle<>> handles;

    void schedule(stdx::coroutine_handle<> handle) {
        handles.push_back(handle);
    }
    void unschedule(stdx::coroutine_handle<> handle) {
        auto it = std::find(handles.rbegin(), handles.rend(), handle);
        VERIFY(Event, it != handles.rend());
        handles.erase(handles.rend() - it + handles.begin());
    }
};
inline Executor* currentExecutor() { return (Executor*)plt::getThreadLocalPtr<plt::TLS_CURRENT_EXECUTOR_INDEX>(); }
inline void setCurrentExecutor(Executor* exe) { plt::setThreadLocalPtr<plt::TLS_CURRENT_EXECUTOR_INDEX>(exe); }
struct ExecutorImpl : Executor {
    Executor nextTick;
    void execute() {
        setCurrentExecutor(this);
        for (int_t i = 0; i < (int_t)handles.size(); i++)
            handles[i].resume();
        handles = std::move(nextTick.handles);
        setCurrentExecutor(nullptr);
    }
};

template<typename E = void>
struct Signal;

template<>
struct Signal<void> {
    Script script;
    uint64_t mask = 0;

    void send(int_t count = 1) const;

    bool operator==(const Script& script) const { return this->script == script; }

    Signal<> operator~() const { return { script, (uint64_t)-(int64_t)mask }; }
};

void test();

}
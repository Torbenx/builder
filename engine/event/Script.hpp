#pragma once

#include "platform/thread_local_storage.hpp"
#include "util/coroutines.hpp"
#include "util/types.hpp"
#include <vector>

namespace Tag {
struct Event {
    static constexpr char tag[] = "event";
    static constexpr bool verifyEnabled = true;
};
}

namespace evt {

struct Script;
struct ScriptHandle;
struct Executor;

inline Executor* currentExecutor() { return (Executor*)plt::getThreadLocalPtr<plt::TLS_CURRENT_EXECUTOR_INDEX>(); }
inline void setCurrentExecutor(Executor* exe) { plt::setThreadLocalPtr<plt::TLS_CURRENT_EXECUTOR_INDEX>(exe); }

struct AwaitableState {
    uint64_t state = 0;
    uint64_t key = 0;
};

struct ScriptPromise {
    Script get_return_object();
    ScriptHandle scriptHandle();
    void unhandled_exception() { }
    stdx::suspend_never initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }

    AwaitableState state;
    Executor* executor = nullptr;
};
static_assert(std::is_standard_layout_v<ScriptPromise>);
template<typename T>
concept CompatibleScriptPromise = std::is_base_of_v<ScriptPromise, T> && alignof(T) == alignof(ScriptPromise) && sizeof(T) == sizeof(ScriptPromise) && std::is_standard_layout_v<T>;

struct ScriptHandle {
    ScriptHandle() = default;
    ScriptHandle(stdx::coroutine_handle<ScriptPromise> handle)
        : handle(handle) { }
    template<CompatibleScriptPromise Promise>
    ScriptHandle(stdx::coroutine_handle<Promise> handle)
        : handle(stdx::coroutine_handle<ScriptPromise>::from_address(handle.address())) { }
    stdx::coroutine_handle<ScriptPromise> handle = {};
    ScriptPromise* promise() const { return &handle.promise(); }
    ScriptPromise* operator->() const { return promise(); }
    ScriptPromise& operator*() const { return *promise(); }
    explicit operator bool() const { return (bool)handle; }
};

struct Script : ScriptHandle {
    using promise_type = ScriptPromise;
};

inline ScriptHandle ScriptPromise::scriptHandle() { return { stdx::coroutine_handle<ScriptPromise>::from_promise(*this) }; }
inline Script ScriptPromise::get_return_object() { return { scriptHandle() }; }

struct Executor {
    std::vector<ScriptHandle> scripts;
    void execute() {
        for (int_t i = 0; i < (int_t)scripts.size(); i++)  
            scripts[i].handle.resume();
        scripts.clear();
    }
};

struct Signal {
    ScriptHandle script;
    uint64_t mask = 0;

    void notify() const {
        Executor* localExecutor = currentExecutor();
        uint64_t oldState = script->state.state;
        uint64_t newState = oldState + mask;
        script->state.state = newState;
        uint64_t key = script->state.key;
        if ((newState & key) != 0 && (oldState & key) == 0) {
            Executor* exe = script->executor;
            exe = exe ? exe : localExecutor;
            exe->scripts.push_back(script);
        }
    }

    Signal operator~() const { return { script, -mask }; }
};

}
#pragma once

#include "CompiledCondition.hpp"

namespace evt {

struct ScriptCompletionEventSource;

struct AwaitableState {
    uint64_t state = 0;
    uint64_t key = 0;
};
struct ScriptPromise {
    Script get_return_object() { return { script() }; }
    Script script() { return { stdx::coroutine_handle<ScriptPromise>::from_promise(*this) }; }
    void unhandled_exception() { VERIFY_NOT_REACHED(Event); }
    stdx::suspend_never initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }
    ScriptPromise();
    ~ScriptPromise();

    AwaitableState state;
    Executor* executor;
    ScriptCompletionEventSource* completion;
};
static_assert(std::is_standard_layout_v<ScriptPromise>);

}
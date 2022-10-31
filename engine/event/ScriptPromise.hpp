#pragma once

#include "Event.hpp"

namespace evt {

struct ScriptCompletionEvent {
    // FIXME: this is broken
    static constexpr bool NO_CLEANUP_IN_CANCEL = true;
    using EventReceiver = KeepLastReceiver<ScriptCompletionEvent>;
};

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
    ~ScriptPromise();

    AwaitableState state;
    bool executeOnNextTick = false;
    Source<ScriptCompletionEvent> completion;
};

inline ScriptPromise* Script::promise() const { return &handle.promise(); }
inline ScriptPromise* Script::operator->() const { return promise(); }
inline ScriptPromise& Script::operator*() const { return *promise(); }

}
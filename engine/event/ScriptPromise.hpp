#pragma once

#include "CompiledCondition.hpp"

namespace evt {

struct ScriptCompletionEvent {
    mutable std::vector<Signal<>> signals;

    struct NodeType : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        const ScriptCompletionEvent* event = nullptr;
        void commit(Signal<> signal) const {
            event->signals.push_back(signal);
        }
    };
    constexpr operator NodeType() const { return { {}, this }; }
    void send() const {
        for (Signal<> s : signals)
            s.send();
    }
};

struct AwaitableState {
    uint64_t state = 0;
    uint64_t key = 0;
};
struct ScriptPromise {
    Script get_return_object() { return { Script() }; }
    Script Script() { return { stdx::coroutine_handle<ScriptPromise>::from_promise(*this) }; }
    void unhandled_exception() { VERIFY_NOT_REACHED(Event); }
    stdx::suspend_never initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }
    ~ScriptPromise() { completion.send(); }

    AwaitableState state;
    Executor* executor = nullptr;
    ScriptCompletionEvent completion;
};
static_assert(std::is_standard_layout_v<ScriptPromise>);

}
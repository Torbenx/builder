#pragma once

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
struct Executor;

struct ScriptPromise {
    Script get_return_object();
    void unhandled_exception() { }
    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }

    uint64_t state = 0;
    uint64_t key = 0;
    Executor* executor = nullptr;
};

struct ScriptHandle {
    stdx::coroutine_handle<ScriptPromise> handle = {};
    ScriptPromise* promise() const { return &handle.promise(); }
    ScriptPromise* operator->() const { return promise(); }
    ScriptPromise& operator*() const { return *promise(); }
    explicit operator bool() const { return (bool)handle; }
};

struct Script : ScriptHandle {
    using promise_type = ScriptPromise;
};

inline Script ScriptPromise::get_return_object() { return { { stdx::coroutine_handle<ScriptPromise>::from_promise(*this) } }; }

struct Executor {
    std::vector<ScriptHandle> scripts;
};
struct InlineExecutor : Executor {
    void execute() {
        for (ScriptHandle scr : scripts)
            scr.handle.resume();
        scripts.clear();
    }
    ~InlineExecutor() {
        VERIFY(Event, scripts.empty());
    }
};

struct Signal {
    ScriptHandle script;
    uint64_t mask = 0;

    void notify(Executor* localExecutor) const {
        uint64_t oldState = script->state;
        uint64_t newState = oldState + mask;
        script->state = newState;
        uint64_t key = script->key;
        if ((newState & key) != 0 && (oldState & key) == 0) {
            Executor* exe = script->executor;
            exe = exe ? exe : localExecutor;
            exe->scripts.push_back(script);
        }
    }
    void notify() const;

    Signal operator~() const { return { script, -mask }; }
};

}
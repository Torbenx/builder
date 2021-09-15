#pragma once

#include "events/Scheduler.hpp"

template<typename T>
struct await_value {
    T t;
    bool await_ready() { return true; }
    bool await_suspend(stdx::coroutine_handle<>) { return false; }
    T await_resume() { return t; }
};

struct get_self_t { };
static constexpr get_self_t get_self = {};

template<typename Interface = void>
class Script;

template<typename Interface = void>
class ScriptPromise {
public:
    template<typename... Args>
    ScriptPromise(Args&... args) { }
    ScriptPromise(const ScriptPromise&) = delete;

    Script<Interface> get_return_object() { return Script<Interface> { this }; }

    auto await_transform(get_self_t) {
        return await_value<ScriptPromise&> { *this };
    }

    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Events);
        std::abort();
    }
};

template<typename Interface>
class Script {
public:
    using promise_type = ScriptPromise<Interface>;

    promise_type* promise;
};

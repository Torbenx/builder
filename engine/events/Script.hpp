#pragma once

#include "events/Scheduler.hpp"
#include <type_traits>

#ifdef interface
// apparently mingw's rcp.h defines this
#undef interface
#endif

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
class future_script;
template<typename Interface = void>
class past_script;
template<typename Interface = void>
class ScriptPromise;

template<>
class ScriptPromise<void> {
public:
    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    template<typename... Args>
    explicit ScriptPromise(Args&... args) { }

    ScriptPromise(const ScriptPromise&) = delete;
    ScriptPromise(ScriptPromise&&) = delete;
    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Events);
        std::abort();
    }
};

template<typename Interface>
class ScriptPromise : public ScriptPromise<void> {
public:
    Interface interface;

    using ScriptPromise<void>::ScriptPromise;
    using ScriptPromise<void>::await_transform;

    auto await_transform(get_self_t) {
        return await_value<ScriptPromise&> { *this };
    }
    Script<Interface> get_return_object() { return { this }; }

    template<typename Inf>
    future_script<Inf> acquire_future(Script<Inf> script) { return { this, script.promise }; }
    template<typename Inf>
    past_script<Inf> acquire_past(Script<Inf> script) { return { this, script.promise }; }
};

template<typename Interface>
class Script {
public:
    using promise_type = ScriptPromise<Interface>;
    operator Script<void>() { return { promise }; }

private:
    template<typename Inf>
    friend class ScriptPromise;
    Script(ScriptPromise<Interface>* promise)
        : promise(promise) { }
    ScriptPromise<Interface>* promise;
};

template<typename Interface>
class script_handle {
public:
    std::add_lvalue_reference_t<Interface> interface() requires(!std::is_same_v<Interface, void>) {
        return target->interface;
    }
    script_handle() { }
    script_handle(ScriptPromise<>* base, ScriptPromise<Interface>* target)
        : base(base), target(target) { }
    script_handle(const script_handle&) = delete;
    script_handle(script_handle&& other) {

        base = other.base;
        target = other.target;
        other.base = nullptr;
        other.target = nullptr;
    }

private:
    ScriptPromise<>* base = nullptr;
    ScriptPromise<Interface>* target = nullptr;
};

template<typename Interface>
struct future_script : script_handle<Interface> {
    using script_handle<Interface>::script_handle;
};
template<typename Interface>
struct past_script : script_handle<Interface> {
    using script_handle<Interface>::script_handle;
};

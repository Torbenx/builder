#pragma once

#include "events/Scheduler.hpp"
#include "containers/AtomicArrayBuilder.hpp"
#include <type_traits>

#ifdef interface
// apparently mingw's rcp.h defines this
#undef interface
#endif

namespace Tag {
struct Script {
    static constexpr char tag[] = "script";
    static constexpr bool verifyEnabled = true;
};
}

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
class ScriptPromise;

template<typename Interface = void>
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
        free();
        base = other.base;
        target = other.target;
        other.base = nullptr;
        other.target = nullptr;
    }
    ~script_handle() { free(); }

    operator script_handle<void>() { return { base, target }; }

private:
    ScriptPromise<>* base = nullptr;
    ScriptPromise<Interface>* target = nullptr;

    void free() {

    }
};

template<typename Interface = void>
struct future_script : script_handle<Interface> {
    using script_handle<Interface>::script_handle;
};
template<typename Interface = void>
struct past_script : script_handle<Interface> {
    using script_handle<Interface>::script_handle;
};

class ScriptRef {
public:
    ScriptRef() { }
    ScriptRef(ScriptPromise<>* ptr, bool heldByMe, bool heldByOther)
        : data(((uintptr_t)ptr) | (heldByMe ? HELD_BY_ME_BIT : 0) | (heldByOther ? HELD_BY_OTHER_BIT : 0)) { }

    static ScriptRef heldByMe(ScriptPromise<>* ptr) { return { ptr, true, false }; }
    static ScriptRef heldByOther(ScriptPromise<>* ptr) { return { ptr, false, true }; }
    static ScriptRef heldByBoth(ScriptPromise<>* ptr) { return { ptr, true, true }; }

    ScriptPromise<>* ptr() { return (ScriptPromise<>*)(data & ~EXTRA_BITS); }
    bool heldByMe() { return data & HELD_BY_ME_BIT; }
    bool heldByOther() { return data & HELD_BY_OTHER_BIT; }
    bool held() { return data & HELD_BITS; }
    void acquireByMe() { data |= HELD_BY_ME_BIT; }
    void acquireByOther() { data |= HELD_BY_OTHER_BIT; }
    void releaseByMe() { data &= ~HELD_BY_ME_BIT; }
    void releaseByOther() { data &= ~HELD_BY_OTHER_BIT; }

private:
    static constexpr uintptr_t HELD_BY_ME_BIT = 0x1;
    static constexpr uintptr_t HELD_BY_OTHER_BIT = 0x2;
    static constexpr uintptr_t HELD_BITS = HELD_BY_OTHER_BIT | HELD_BY_ME_BIT;
    static constexpr uintptr_t EXTRA_BITS = HELD_BITS;
    uintptr_t data = 0;
};

template<>
class ScriptPromise<void> {
public:
    template<typename... Args>
    explicit ScriptPromise(Args&... args) { }
    ScriptPromise(const ScriptPromise&) = delete;
    ScriptPromise(ScriptPromise&&) = delete;

    template<typename Inf>
    future_script<Inf> acquire_future(Script<Inf> script) {
        return { this, script.promise };
    }
    template<typename Inf>
    past_script<Inf> acquire_past(Script<Inf> script) {
        return { this, script.promise };
    }

    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Events);
        std::abort();
        dependencies.pushBack(ScriptRef::heldByMe(nullptr));
    }

private:
    AtomicArrayBuilder<ScriptRef> dependencies;
};

template<typename Interface>
struct ScriptInterfaceHolder {
    Interface interface;
};
template<>
struct ScriptInterfaceHolder<void> { };

template<typename Interface>
class ScriptPromise : public ScriptPromise<void> {
public:
    Interface interface;

    using ScriptPromise<void>::ScriptPromise;
    using ScriptPromise<void>::await_transform;

    auto await_transform(get_self_t) { return await_value<ScriptPromise&> { *this }; }
    Script<Interface> get_return_object() { return { this }; }

    future_script<Interface> flip(past_script<> script) {
        VERIFY(Script, script.base == this);
        return { script.target, this };
    }
    past_script<Interface> flip(future_script<> script) {
        VERIFY(Script, script.base == this);
        return { script.target, this };
    }
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

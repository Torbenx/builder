#pragma once

#include "containers/MemoryAllocator32.hpp"
#include "events/Scheduler.hpp"
#include "util/log.hpp"
#include "util/uninitialized.hpp"
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

namespace script_detail {
inline uninitialized<MemoryAllocator32> g_allocator;
inline MemoryAllocator32& allocator() { return g_allocator.value; }
}

template<typename Interface = void>
class ScriptPromise;
template<typename Interface = void>
class ScriptPromiseBase;
class ScriptDependency;

template<typename Interface = void>
class Script {
public:
    using promise_type = ScriptPromise<Interface>;

    Script() { }
    operator Script<void>() const { return { offset }; }
    ScriptPromise<Interface>* operator->() { return (ScriptPromise<Interface>*)script_detail::allocator().address(offset); }
    ScriptPromise<Interface>& operator*() { return *(ScriptPromise<Interface>*)script_detail::allocator().address(offset); }
    explicit operator bool() const { return offset != 0; }
    bool operator==(const Script& other) { return offset == other.offset; }

private:
    template<typename Inf>
    friend class Script;
    template<typename Inf>
    friend class ScriptPromise;
    template<typename Inf>
    friend class ScriptPromiseBase;
    friend class ScriptDependency;

    uint32_t offset = 0;

    Script(uint32_t offset)
        : offset(offset) { }
};

template<typename Interface = void>
class dependent_script {
public:
    std::add_lvalue_reference_t<Interface> interface() requires(!std::is_same_v<Interface, void>) {
        return target->interface;
    }
    dependent_script() { }
    dependent_script(Script<> base, Script<Interface> target)
        : base(base), target(target) { }
    dependent_script(const dependent_script&) = delete;
    dependent_script(dependent_script&& other) {
        free();
        base = other.base;
        target = other.target;
        other.base = nullptr;
        other.target = nullptr;
    }
    ~dependent_script() { free(); }

    operator dependent_script<void>() { return { base, target }; }

private:
    Script<> base = {};
    Script<Interface> target = {};

    void free() { }
};

template<typename Interface = void>
struct future_script : dependent_script<Interface> {
    using dependent_script<Interface>::dependent_script;
};
template<typename Interface = void>
struct past_script : dependent_script<Interface> {
    using dependent_script<Interface>::dependent_script;
};

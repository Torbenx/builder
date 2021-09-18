#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/MemoryAllocator32.hpp"
#include "events/Scheduler.hpp"
#include "util/log.hpp"
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
union helper_union {
    MemoryAllocator32 allocator;
    struct {
    } dummy;

    helper_union()
        : dummy() { }

    ~helper_union() {
        allocator.~MemoryAllocator32();
    }
};
inline helper_union allocator;
}

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
    ScriptPromise<>* base = 0;
    ScriptPromise<Interface>* target = 0;

    void free() { }
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
    static constexpr size_t MAX_REF_COUNT = 256;
    ScriptRef() { }
    ScriptRef(ScriptPromise<>* ptr, uintptr_t initialRefCount = 1)
        : data((uintptr_t)ptr | (uintptr_t)initialRefCount) { }

    ScriptPromise<>* ptr() { return (ScriptPromise<>*)(data.load(std::memory_order_relaxed) & PTR_MASK); }
    size_t refCount() { return data.load(std::memory_order_relaxed) & REF_COUNT_MASK; }
    size_t ref(size_t count = 1) { return data.fetch_add(count, std::memory_order_relaxed) & REF_COUNT_MASK; }
    size_t unref(size_t count = 1) { return data.fetch_sub(count, std::memory_order_relaxed) & REF_COUNT_MASK; }

private:
    static constexpr uintptr_t REF_COUNT_MASK = MAX_REF_COUNT - 1;
    static constexpr uintptr_t PTR_MASK = ~REF_COUNT_MASK;
    std::atomic<uintptr_t> data = 0;
};

template<>
class ScriptPromise<void> {
public:
    void* operator new(size_t size) {
        MemoryAllocator32& alloc = allocator();
        void* addr = alloc.address(alloc.allocate(decltype(insertions)::BLOCK_ALIGNMENT, size));
        alloc.debug();
        return addr;
    }

    void operator delete(void* addr) {
        MemoryAllocator32& alloc = allocator();
        VERIFY(Script, alloc.ownsAddress(addr));
        alloc.free(alloc.offset(addr));
    }

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
    }

private:
    static MemoryAllocator32& allocator() {
        return script_detail::allocator.allocator;
    }

    static_assert(AtomicArrayBuilder<ScriptPromise<>*>::BLOCK_ALIGNMENT == ScriptRef::MAX_REF_COUNT);
    AtomicArrayBuilder<ScriptPromise<>*> insertions;
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

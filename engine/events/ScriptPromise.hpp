#pragma once

#include "containers/AtomicArrayBuilder.hpp"
#include "containers/FlatSet.hpp"
#include "events/Script.hpp"

class ScriptDependency {
public:
    ScriptDependency() { }
    ScriptDependency(Script<> script, size_t initialRefCount = 1)
        : data(((uint64_t)script.offset << 32) | initialRefCount) { }

    Script<> script() const { return { (uint32_t)(data.load(std::memory_order_relaxed) >> 32) }; }
    size_t refCount() const { return data.load(std::memory_order_relaxed) & REF_COUNT_MASK; }
    size_t ref(size_t count = 1) { return data.fetch_add(count, std::memory_order_relaxed) & REF_COUNT_MASK; }
    size_t unref(size_t count = 1) { return data.fetch_sub(count, std::memory_order_relaxed) & REF_COUNT_MASK; }

    bool operator==(const ScriptDependency& other) const { return script() == other.script(); }

private:
    static constexpr uint64_t REF_COUNT_MASK = std::numeric_limits<uint32_t>::max();
    std::atomic<uint64_t> data = 0;
};

template<>
class ScriptPromiseBase<void> {
public:
    void* operator new(size_t size) {
        MemoryAllocator32& alloc = script_detail::allocator();
        void* addr = alloc.address(alloc.allocate(decltype(insertions)::BLOCK_ALIGNMENT, size));
        alloc.debug();
        return addr;
    }

    void operator delete(void* addr) {
        MemoryAllocator32& alloc = script_detail::allocator();
        VERIFY(Script, alloc.ownsAddress(addr));
        alloc.free(alloc.offset(addr));
    }

    template<typename... Args>
    explicit ScriptPromiseBase(Args&... args) { }
    ScriptPromiseBase(const ScriptPromiseBase&) = delete;
    ScriptPromiseBase(ScriptPromiseBase&&) = delete;

    auto initial_suspend() { return stdx::suspend_never(); }
    auto final_suspend() noexcept { return stdx::suspend_never(); }
    void return_void() { }
    void unhandled_exception() {
        VERIFY_NOT_REACHED(Events);
        std::abort();
    }

protected:
    AtomicArrayBuilder<Script<>> insertions;
    FlatSet<ScriptDependency> dependencies;
};

template<typename Interface>
class ScriptPromiseBase : public ScriptPromiseBase<void> {
public:
    Interface interface;

    using ScriptPromiseBase<void>::ScriptPromiseBase;
};

template<typename Interface>
struct ScriptPromise : ScriptPromiseBase<Interface> {
    using ScriptPromiseBase<Interface>::ScriptPromiseBase;

    Script<Interface> script() { return { script_detail::allocator().offset(this) }; }

    await_value<ScriptPromise&> await_transform(get_self_t) { return { *this }; }
    template<typename T>
    decltype(auto) await_transform(T&& t) { return std::forward<T>(t); }

    Script<Interface> get_return_object() {
        VERIFY(Script, script_detail::allocator().ownsAddress(this));
        return script();
    }

    template<typename Inf>
    future_script<Inf> acquire_future(Script<Inf> script) {
        return { this->script(), script };
    }
    template<typename Inf>
    past_script<Inf> acquire_past(Script<Inf> script) {
        return { this->script(), script };
    }

    future_script<Interface> flip(past_script<> script) {
        VERIFY(Script, script.base == this);
        return { script.target, this };
    }
    past_script<Interface> flip(future_script<> script) {
        VERIFY(Script, script.base == this);
        return { script.target, this };
    }
};
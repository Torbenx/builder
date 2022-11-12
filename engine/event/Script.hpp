#pragma once

#include "platform/thread_local_storage.hpp"
#include "util/coroutines.hpp"
#include <algorithm>
#include <vector>

namespace Tag {
struct Event {
    static constexpr char tag[] = "event";
    static constexpr bool verifyEnabled = true;
};
}

namespace evt {

constexpr int_t MAX_CONCURRENT_TICKS = 7;

DECLARE_THREAD_LOCAL_GETSET(int_t, mainTick, setMainTick, MAIN_TICK)

struct ScriptPromise;

template<typename T, typename Base>
concept CompatibleCoroutinePromise = std::is_base_of_v<Base, T> && alignof(T) == alignof(Base) && sizeof(T) == sizeof(Base);
struct Script {
    using promise_type = ScriptPromise;
    Script() = default;
    Script(stdx::coroutine_handle<ScriptPromise> handle)
        : handle(handle) { }
    template<CompatibleCoroutinePromise<ScriptPromise> Promise>
    Script(stdx::coroutine_handle<Promise> handle)
        : handle(stdx::coroutine_handle<ScriptPromise>::from_address(handle.address())) { }

    static Script fromAddress(void* address) {
        return stdx::coroutine_handle<ScriptPromise>::from_address(address);
    }

    stdx::coroutine_handle<ScriptPromise> handle = {};

    void* address() const { return handle.address(); }
    ScriptPromise* promise() const;
    ScriptPromise* operator->() const;
    ScriptPromise& operator*() const;
    explicit operator bool() const { return (bool)handle; }
    operator stdx::coroutine_handle<>() const { return handle; }
    bool operator==(const Script&) const = default;
    void deallocate() {
        if (handle) {
            handle.destroy();
            handle = {};
        }
    }
};

struct Executor {
    std::vector<stdx::coroutine_handle<>> handles;

    void schedule(stdx::coroutine_handle<> handle) {
        handles.push_back(handle);
    }
    void unschedule(stdx::coroutine_handle<> handle) {
        auto it = std::find(handles.rbegin(), handles.rend(), handle);
        VERIFY(Event, it != handles.rend());
        handles.erase(handles.rend() - it + handles.begin());
    }
};
struct ExecutorImpl : Executor {
    Executor nextTick;
    int_t execute() {
        int_t i = 0;
        for (; i < (int_t)handles.size(); i++)
            handles[i].resume();
        handles = std::move(nextTick.handles);
        return i;
    }
};

template<typename E = void>
struct Signal;

template<>
struct Signal<void> {
    Script script;
    uint64_t mask = 0;

    void send(int_t count = 1) const;

    bool operator==(const Script& script) const { return this->script == script; }

    Signal<> operator~() const { return { script, (uint64_t)(-(int64_t)mask) }; }
};

static constexpr uint64_t UNBOUND_EVENT_COUNT = 1000;

struct TmplLeafNode {
    static constexpr uint64_t requiredEvents() { return 0; }
    static constexpr uint64_t maxEvents() { return 0; }

    uint64_t state() const { return 0; }
    void commit(Signal<>) const { }
    void cancel(Script) const { }
};

void test();

}
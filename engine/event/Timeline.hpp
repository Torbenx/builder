#pragma once

#include "Event.hpp"
#include <atomic>

namespace evt {

void async(stx::BasicCallback<>, int_t tick = evt::mainTick());
void postCallback(Source<CallbackEvent>& postTo, stx::BasicCallback<>);
bool schedulerCheck();

struct SchedulerCheck {
    bool await_ready() {
        return !schedulerCheck();
    }
    void await_suspend(stdx::coroutine_handle<> handle) {
        async({
            handle.address(),
            [](void* addr) { stdx::coroutine_handle<>::from_address(addr).resume(); },
        });
    }
    void await_resume() { }
};

class EngineScope {
private:
    void* data = nullptr;

public:
    EngineScope();
    ~EngineScope();
    static bool run();
};

template<typename T, int_t N>
struct RingBuffer {
    T storage[N] = {};
    int_t index(int_t i) const { return (size_t)i % N; }
    constexpr T& operator[](int_t i) { return storage[index(i)]; }
    constexpr const T& operator[](int_t i) const { return storage[index(i)]; }
    constexpr int_t size() { return N; }
};
template<typename T>
using TickRingBuffer = RingBuffer<T, MAX_CONCURRENT_TICKS + 1>;
template<typename E>
using EventRingBuffer = TickRingBuffer<Receiver<E>>;

class Timeline {
public:
    static constexpr int_t INVALID_TICK = std::numeric_limits<int_t>::min();
    // local
    ExecutorImpl m_executor;
    int_t lastStartedTick = -1;

    void beginTick();
    int_t executeScripts() { return executor()->execute(); }
    void clearTick();
    void endTick();

    // shared
    std::atomic<int_t> lastScheduledTick = INVALID_TICK;
    TickRingBuffer<int_t> tickMapping;
    stx::BasicCallback<> asyncCallback;

    // remote
    Source<CallbackEvent> m_callbackHandler;

public:
    ExecutorImpl* executor() { return &m_executor; }
    Source<CallbackEvent>& callbackHandler() { return m_callbackHandler; }
    int_t tick() const { return lastStartedTick; }
};
DECLARE_THREAD_LOCAL_GETSET(Timeline*, currentTimeline, setCurrentTimeline, TIMELINE)

template<std::derived_from<Timeline> TL>
class AsyncTimeline;

}
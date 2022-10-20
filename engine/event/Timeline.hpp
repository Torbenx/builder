#pragma once

#include "Event.hpp"
#include <atomic>

namespace evt {

void completeStartup(stx::BasicCallback<> callback);

void async(stx::BasicCallback<>, int_t tick = evt::mainTick());

void postCallback(Source<CallbackEvent>& postTo, stx::BasicCallback<>);

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

struct RelayPromise {
    Executor* executor = nullptr;
    void startRelay(Executor* executor, stdx::coroutine_handle<> handle) {
        this->executor = executor;
        executor->schedule(handle);
    }
    void reschedule(stdx::coroutine_handle<> handle) {
        static_cast<ExecutorImpl*>(executor)->nextTick.schedule(handle);
    }
    stdx::suspend_always initial_suspend() { return {}; }
    stdx::suspend_never final_suspend() noexcept { return {}; }
    void return_void() { }
    void unhandled_exception() { VERIFY_NOT_REACHED(Event); }
};

}

template<std::derived_from<evt::RelayPromise> T>
struct stx::DeallocateCall<T*> {
    struct type {
        constexpr void operator()(T*& promise) const {
            if (promise) {
                promise->handle().destroy();
                promise = nullptr;
            }
        }
    };
};

namespace evt {

template<typename... Es>
struct RelayOutputPromise : RelayPromise {
    stx::Tuple<EventRingBuffer<Es>...> buffers;
    stx::Tuple<Source<Es>...> outputs;
    RelayOutputPromise(stx::int_sequence_for<Es...>, RelayOutputPromise*& promise) { promise = this; }
    auto handle() { return stdx::coroutine_handle<RelayOutputPromise>::from_promise(*this); }
    RelayOutputPromise* get_return_object() { return this; }
    void startRelay(Executor* exe) { RelayPromise::startRelay(exe, handle()); }
    void reschedule() { RelayPromise::reschedule(handle()); }

    template<typename E>
    Source<E>& output() { return stx::get<Source<E>>(outputs); }
    template<typename E>
    const Source<E>& output() const { return stx::get<Source<E>>(outputs); }
    template<int_t i>
    auto& output() { return stx::get<i>(outputs); }
    template<int_t i>
    const auto& output() const { return stx::get<i>(outputs); }

    template<typename E>
    EventRingBuffer<E>& buffer() { return stx::get<EventRingBuffer<E>>(buffers); }
    template<typename E>
    const EventRingBuffer<E>& buffer() const { return stx::get<EventRingBuffer<E>>(buffers); }
    template<int_t i>
    auto& buffer() { return stx::get<i>(buffers); }
    template<int_t i>
    const auto& buffer() const { return stx::get<i>(buffers); }

    template<typename E>
    Receiver<E> takeEvents() {
        return std::move(buffer<E>()[evt::mainTick()]);
    }
    template<typename E>
    Receiver<E> sendEvents() {
        Receiver<E> events = std::move(buffer<E>()[evt::mainTick()]);
        output<E>().sendEvents(events);
        return events;
    }
    template<int_t i>
    auto takeEvents() {
        return std::move(buffer<i>()[evt::mainTick()]);
    }
    template<int_t i>
    auto sendEvents() {
        auto events = std::move(buffer<i>()[evt::mainTick()]);
        output<i>().sendEvents(events);
        return events;
    }
};

namespace detail {
    template<typename... Es, int_t... I>
    RelayOutputPromise<Es...>* broadcastEvents(stx::int_sequence<I...>, RelayOutputPromise<Es...>* promise = {}) {
        while (true) {
            (promise->template sendEvents<I>(), ...);
            promise->reschedule();
            co_await stdx::suspend_always();
        }
    }
    template<typename... Es, int_t... I>
    Script recordEvents(RelayOutputPromise<Es...>* output, stx::int_sequence<I...>, const SourceImpl<Es>&... sources) {
        while (true) {
            stx::Tuple<Receiver<Es>...> receivers {};
            await { ((sources > stx::get<I>(receivers)) || ...) };
            int_t tick = evt::mainTick();
            ((output->template buffer<I>()[tick] = std::move(stx::get<I>(receivers))), ...);
        }
    }
}
template<typename... Es>
struct Relay {
    stx::Owned<Script> input;
    stx::Owned<RelayOutputPromise<Es...>*> output;

    [[nodiscard]] stx::Owned<RelayOutputPromise<Es...>*> broadcastEvents(Executor* executor) {
        VERIFY(Event, (bool)output);
        output->startRelay(executor);
        return std::move(output);
    }
};
template<typename... Es>
[[nodiscard]] Relay<Es...> relayEvents(const SourceImpl<Es>&... sources) {
    RelayOutputPromise<Es...>* output = detail::broadcastEvents<Es...>(stx::int_sequence_for<Es...>());
    Script input = detail::recordEvents(output, stx::int_sequence_for<Es...>(), sources...);
    return { input, output };
}

class RemoteTimeline;

class Timeline {
public:
    friend class RemoteTimeline;
    static constexpr int_t INVALID_TICK = std::numeric_limits<int_t>::min();
    // local
    ExecutorImpl m_executor;
    int_t lastStartedTick = -1;

    void beginTick();
    void executeScripts() { executor()->execute(); }
    void clearTick() { setCurrentTimline(nullptr); }
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
Timeline* mainTimeline();
DECLARE_THREAD_LOCAL_GETSET(Timeline*, currentTimeline, setCurrentTimline, CURRENT_TIMELINE);

class RemoteTimeline {
private:
    Timeline m_timeline;
    Relay<CallbackEvent> callbackRelay = relayEvents(timeline()->callbackHandler());
    int_t nextTickToSchedule = 0;

protected:
    Timeline* timeline() { return &m_timeline; }

public:
    RemoteTimeline();
    void asyncTick();

    Source<CallbackEvent>& callbackHandler() { return timeline()->callbackHandler(); }
};

}
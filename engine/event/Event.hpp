#pragma once

#include "Awaitable.hpp"
#include "CompiledCondition.hpp"
#include "Scheduler.hpp"
#include "containers/Tuple.hpp"

namespace evt {

template<typename E>
struct EventReceiver {
    std::vector<E> events;
};

template<typename E>
struct EventSignal {
    Signal signal;
    EventReceiver<E>* receiver = nullptr;
    void send(const E& e) const {
        receiver->events.emplace_back(e);
        signal.send();
    }
    void send(ArrayView<const E> events) const {
        receiver->events.insert(receiver->events.end(), events.begin(), events.end());
        signal.send(events.size());
    }
    bool operator==(const ScriptHandle& script) const { return signal == script; }
};

template<typename E>
struct EventSource {
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        const EventSource* source = nullptr;
        EventReceiver<E>* receiver = nullptr;

        void commit(Signal signal) const {
            source->signals.push_back({ signal, receiver });
        }
        void cancel(ScriptHandle script) const {
            auto it = std::remove(source->signals.begin(), source->signals.end(), script);
            source->signals.erase(it, source->signals.end());
        }
    };
    mutable std::vector<EventSignal<E>> signals;
    EventSource() = default;
    EventSource(const EventSource&) = delete;
    constexpr NodeType operator>(EventReceiver<E>& rec) const {
        return { {}, this, &rec };
    }
    void sendEvent(const E& event) {
        for (const EventSignal<E>& signal : signals)
            signal.send(event);
    }
    void sendEvents(ArrayView<const E> events) {
        for (const EventSignal<E>& signal : signals)
            signal.send(events);
    }
};

template<typename E>
using EventRingBuffer = TickRingBuffer<std::vector<E>>;

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
    void unhandled_exception() { }
};

template<typename... Es>
struct RelayOutputPromise : RelayPromise {
    stx::Tuple<EventRingBuffer<Es>...> buffers;
    stx::Tuple<EventSource<Es>...> outputs;
    RelayOutputPromise(stx::int_sequence_for<Es...>, RelayOutputPromise*& promise) { promise = this; }
    auto handle() { return stdx::coroutine_handle<RelayOutputPromise>::from_promise(*this); }
    RelayOutputPromise* get_return_object() { return this; }
    void startRelay(Executor* exe) { RelayPromise::startRelay(exe, handle()); }
    void reschedule() { RelayPromise::reschedule(handle()); }

    template<typename E>
    EventSource<E>& output() { return stx::get<EventSource<E>>(outputs); }
    template<typename E>
    const EventSource<E>& output() const { return stx::get<EventSource<E>>(outputs); }
    template<int_t i>
    auto& output() { return stx::get<i>(outputs); }
    template<int_t i>
    const auto& output() const { return stx::get<i>(outputs); }

    template<typename E>
    auto& buffer() { return stx::get<EventRingBuffer<E>>(buffers); }
    template<typename E>
    const auto& buffer() const { return stx::get<EventRingBuffer<E>>(buffers); }
    template<int_t i>
    auto& buffer() { return stx::get<i>(buffers); }
    template<int_t i>
    const auto& buffer() const { return stx::get<i>(buffers); }

    template<typename E>
    std::vector<E> takeEvents() {
        return std::move(buffer<E>()[tick()]);
    }
    template<typename E>
    std::vector<E> sendEvents() {
        std::vector<E> events = std::move(buffer<E>()[tick()]);
        output<E>().sendEvents(events);
        return events;
    }
    template<int_t i>
    auto takeEvents() {
        return std::move(buffer<i>()[tick()]);
    }
    template<int_t i>
    auto sendEvents() {
        auto events = std::move(buffer<i>()[tick()]);
        output<i>().sendEvents(events);
        return events;
    }
};

template<typename... Es, int_t... I>
RelayOutputPromise<Es...>* broadcastEvents(stx::int_sequence<I...>, RelayOutputPromise<Es...>* promise = {}) {
    while (true) {
        (promise->template sendEvents<I>(), ...);
        promise->reschedule();
        co_await stdx::suspend_always();
    }
}
template<typename... Es, int_t... I>
Script recordEvents(RelayOutputPromise<Es...>* output, stx::int_sequence<I...>, const EventSource<Es>&... sources) {
    while (true) {
        stx::Tuple<EventReceiver<Es>...> receivers;
        co_await ((sources > stx::get<I>(receivers)) || ...);
        int_t tick = evt::tick();
        ((stx::get<I>(output->buffers)[tick] = std::move(stx::get<I>(receivers).events)), ...);
    }
}
template<typename... Es>
RelayOutputPromise<Es...>* relayEvents(const EventSource<Es>&... sources) {
    RelayOutputPromise<Es...>* output = broadcastEvents<Es...>(stx::int_sequence_for<Es...>());
    recordEvents(output, stx::int_sequence_for<Es...>(), sources...);
    return output;
}

}
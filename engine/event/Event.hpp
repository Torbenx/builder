#pragma once

#include "CompiledCondition.hpp"
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
};

template<typename T, int_t N>
struct RingBuffer {
    T storage[N] = {};
    int_t index(int_t i) const { return (size_t)i % N; }
    constexpr T& operator[](int_t i) { return storage[index(i)]; }
    constexpr const T& operator[](int_t i) const { return storage[index(i)]; }
};

constexpr int_t MAX_CONCURRENT_TICKS = 8;

template<typename E>
struct EventRelay {
    RingBuffer<std::vector<E>, MAX_CONCURRENT_TICKS> buffer;
    EventSource<E> source;
    std::vector<E> relay() {
        std::vector<E> events = std::move(buffer[tick()]);
        for (const E& event : events)
            source.sendEvent(event);
        return events;
    }
};

template<typename... Es>
struct EventRelayScript {
    struct Promise {
        stx::Tuple<EventRelay<Es>...> relays;
        Executor* executor = nullptr;
        Promise(Promise*& promise) { promise = this; }
        void reschedule() {
            static_cast<ExecutorImpl*>(executor)->nextTick.schedule(handle());
        }
        void startRelay(Executor* executor) {
            this->executor = executor;
            executor->schedule(handle());
        }
        EventRelayScript get_return_object() { return { handle() }; }
        stdx::coroutine_handle<Promise> handle() { return stdx::coroutine_handle<Promise>::from_promise(*this); }
        stdx::suspend_always initial_suspend() { return {}; }
        stdx::suspend_never final_suspend() noexcept { return {}; }
        void return_void() { }
        void unhandled_exception() { }
        template<typename E>
        EventRelay<E>& relay() { return stx::get<EventRelay<E>>(relays); }
        template<typename E>
        const EventRelay<E>& relay() const { return stx::get<EventRelay<E>>(relays); }
    };
    using promise_type = Promise;
    stdx::coroutine_handle<Promise> handle;
};
template<typename... Es>
inline EventRelayScript<Es...> relayEvents(typename EventRelayScript<Es...>::Promise* promise = {}) {
    while (true) {
        stx::forEachElement(promise->relays, []<typename E>(EventRelay<E>& relay) {
            relay.relay();
        });
        promise->reschedule();
        co_await stdx::suspend_always();
    }
}

}
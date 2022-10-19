#pragma once

#include "Awaitable.hpp"
#include "containers/Owned.hpp"
#include "containers/Tuple.hpp"
#include <optional>

namespace evt {

template<typename E>
struct receiver_traits;
template<typename E>
using Receiver = typename receiver_traits<E>::type;
template<typename E>
struct source_traits;
template<typename E>
using Source = typename source_traits<E>::type;

template<typename E>
struct ListReceiver {
    std::vector<E> events;
    void receiveEvent(const E& e) {
        events.push_back(e);
    }
    void receiveEvents(const ListReceiver<E>& other) {
        events.insert(events.end(), other.events.begin(), other.events.end());
    }
    bool hasEvent() const {
        return events.size() > 0;
    }
    int_t count() const {
        return events.size();
    }
    void reset() {
        events.clear();
    }
    template<stx::BasicCallable<void, E> Callback>
    void dispatchTo(const Callback& callback) {
        for (const auto& event : events)
            callback(event);
    }
};
template<typename E>
struct KeepLastReceiver {
    std::optional<E> event;
    KeepLastReceiver() = default;
    KeepLastReceiver(const KeepLastReceiver&) = default;
    KeepLastReceiver(KeepLastReceiver&& other)
        : event(std::move(other.event)) { other.event.reset(); }
    KeepLastReceiver& operator=(const KeepLastReceiver&) = default;
    KeepLastReceiver& operator=(KeepLastReceiver&& other) {
        event = std::move(other.event);
        other.event.reset();
        return *this;
    }
    void receiveEvent(const E& e) {
        event = e;
    }
    void receiveEvents(const KeepLastReceiver<E>& other) {
        VERIFY(Event, other.event.has_value());
        event = other.event.value();
    }
    bool hasEvent() const { return event.has_value(); }
    int_t count() const { return event.has_value(); }
    void reset() { event.reset(); }
    template<stx::BasicCallable<void, E> Callback>
    void dispatchTo(const Callback& callback) {
        if (hasEvent())
            callback(const_cast<const E&>(event.value()));
    }
};

template<typename E>
struct Signal {
    Signal<> signal;
    Receiver<E>* receiver = nullptr;
    void send(const E& e) const {
        receiver->receiveEvent(e);
        signal.send();
    }
    void send(const Receiver<E>& events) const {
        receiver->receiveEvents(events);
        signal.send(events.count());
    }
    bool operator==(const Script& script) const { return signal == script; }
};

// cleanUpInCancel=false is used by SciptCompletionEvent to avoid use after free
template<typename E, bool cleanUpInCancel = true>
struct SourceImpl {
    struct IgnoreContentNode : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        struct ScratchSpace {
            Receiver<E> receiver;
        };

        const SourceImpl& source;

        void commit(Signal<> signal, ScratchSpace& scratch) const {
            source.signals.push_back({ signal, &scratch.receiver });
        }
        void cancel(Script script, ScratchSpace&) const {
            if constexpr (cleanUpInCancel) {
                auto it = std::remove(source.signals.begin(), source.signals.end(), script);
                source.signals.erase(it, source.signals.end());
            }
        }
    };
    struct MoveContentNode : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        const SourceImpl& source;
        Receiver<E>& receiver;

        void commit(Signal<> signal) const {
            source.signals.push_back({ signal, &receiver });
        }
        void cancel(Script script) const {
            if (cleanUpInCancel) {
                auto it = std::remove(source.signals.begin(), source.signals.end(), script);
                source.signals.erase(it, source.signals.end());
            }
        }
    };
    template<typename Callback>
    struct DispatchContentNode : IgnoreContentNode {
        Callback callback;
        void cancel(Script script, typename IgnoreContentNode::ScratchSpace& scratch) const {
            IgnoreContentNode::cancel(script, scratch);
            scratch.receiver.dispatchTo(callback);
        }
    };
    using NodeType = IgnoreContentNode;

    mutable std::vector<Signal<E>> signals;
    SourceImpl() = default;
    SourceImpl(const SourceImpl&) = delete;
    void sendEvent(const E& event) {
        for (const Signal<E>& signal : signals)
            signal.send(event);
    }
    void sendEvents(const Receiver<E>& events) {
        if (!events.hasEvent())
            return;
        for (const Signal<E>& signal : signals)
            signal.send(events);
    }
    constexpr operator NodeType() const {
        return { {}, *this };
    }
};

}

template<typename E, bool b>
constexpr typename evt::SourceImpl<E, b>::MoveContentNode operator>(const evt::SourceImpl<E, b>& source, evt::Receiver<E>& receiver) {
    return { {}, source, receiver };
}
template<typename E, bool b, typename Callback>
constexpr typename evt::SourceImpl<E, b>::template DispatchContentNode<Callback> operator>(const evt::SourceImpl<E, b>& source, const Callback& callback) requires requires(Callback callback, evt::Receiver<E> receiver) { receiver.dispatchTo(callback); }
{
    return { { {}, source }, callback };
}

namespace evt {

template<typename E>
concept HasCustomReceiver = requires { typename E::EventReceiver; };
template<typename E>
struct receiver_traits {
    using type = ListReceiver<E>;
};
template<HasCustomReceiver E>
struct receiver_traits<E> {
    using type = typename E::EventReceiver;
};
template<typename E>
concept HasCustomSource = requires { typename E::EventSource; };
template<typename E>
struct source_traits {
    using type = SourceImpl<E>;
};
template<HasCustomSource E>
struct source_traits<E> {
    using type = typename E::EventSource;
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
        return std::move(buffer<E>()[evt::tick()]);
    }
    template<typename E>
    Receiver<E> sendEvents() {
        Receiver<E> events = std::move(buffer<E>()[evt::tick()]);
        output<E>().sendEvents(events);
        return events;
    }
    template<int_t i>
    auto takeEvents() {
        return std::move(buffer<i>()[evt::tick()]);
    }
    template<int_t i>
    auto sendEvents() {
        auto events = std::move(buffer<i>()[evt::tick()]);
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
            int_t tick = evt::tick();
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

struct CallbackEvent {
    struct EventSource : SourceImpl<CallbackEvent> {
        void sendEvent(stx::BasicCallback<> callback) {
            sendEvent(CallbackEvent { callback });
        }
        void sendEvent(const CallbackEvent& event) {
            if (signals.size() > 0)
                return SourceImpl::sendEvent(event);
            event.callback();
        }
        void sendEvents(const ListReceiver<CallbackEvent>& events) {
            if (signals.size() > 0)
                return SourceImpl::sendEvents(events);
            for (const CallbackEvent& ev : events.events)
                ev.callback();
        }
    };
    stx::BasicCallback<> callback;
};

struct ScriptCompletionEvent {
    using EventReceiver = KeepLastReceiver<ScriptCompletionEvent>;
    using EventSource = ScriptCompletionEventSource;
};
struct ScriptCompletionEventSource : SourceImpl<ScriptCompletionEvent, false> { };

}
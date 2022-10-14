#pragma once

#include "Awaitable.hpp"
#include "CompiledCondition.hpp"
#include "Scheduler.hpp"
#include "containers/Owned.hpp"
#include "containers/Tuple.hpp"
#include <optional>

namespace evt {

template<typename E>
struct traits;
template<typename E>
using Receiver = typename traits<E>::Receiver;
template<typename E>
using Source = typename traits<E>::Source;

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

template<typename E>
struct SourceImpl {
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }

        const SourceImpl* source = nullptr;
        Receiver<E>* receiver = nullptr;

        void commit(Signal<> signal) const {
            source->signals.push_back({ signal, receiver });
        }
        void cancel(Script script) const {
            auto it = std::remove(source->signals.begin(), source->signals.end(), script);
            source->signals.erase(it, source->signals.end());
        }
    };
    mutable std::vector<Signal<E>> signals;
    SourceImpl() = default;
    SourceImpl(const SourceImpl&) = delete;
    constexpr NodeType operator>(Receiver<E>& rec) const {
        return { {}, this, &rec };
    }
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
};

template<typename E>
concept HasCustomReceiver = requires { typename E::EventReceiver; };
template<typename E>
concept HasCustomSource = requires { typename E::EventSource; };
template<typename E>
struct traits;
template<typename E>
requires(!HasCustomReceiver<E> && !HasCustomSource<E>) struct traits<E> {
    using Receiver = ListReceiver<E>;
    using Source = SourceImpl<E>;
};
template<typename E>
requires(HasCustomReceiver<E> && !HasCustomSource<E>) struct traits<E> {
    using Receiver = typename E::EventReceiver;
    using Source = SourceImpl<E>;
};
template<typename E>
requires(!HasCustomReceiver<E> && HasCustomSource<E>) struct traits<E> {
    using Receiver = ListReceiver<E>;
    using Source = typename E::EventSource;
};
template<typename E>
requires(HasCustomReceiver<E>&& HasCustomSource<E>) struct traits<E> {
    using Receiver = typename E::EventReceiver;
    using Source = typename E::EventSource;
};

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
        return std::move(buffer<E>()[tick()]);
    }
    template<typename E>
    Receiver<E> sendEvents() {
        Receiver<E> events = std::move(buffer<E>()[tick()]);
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

}
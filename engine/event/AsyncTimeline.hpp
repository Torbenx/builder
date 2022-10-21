#pragma once

#include "Awaitable.hpp"
#include "Timeline.hpp"

namespace evt {

struct RelayPromiseBase {
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

template<std::derived_from<evt::RelayPromiseBase> T>
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
template<typename Promise>
struct stx::DeallocateCall<stdx::coroutine_handle<Promise>> {
    struct type {
        constexpr void operator()(stdx::coroutine_handle<Promise>& handle) const {
            if (handle) {
                handle.destroy();
                handle = {};
            }
        }
    };
};

namespace evt {

template<typename... Es>
struct RelayPromise : RelayPromiseBase {
    stx::Tuple<EventRingBuffer<Es>...> buffers;
    stx::Tuple<Source<Es>...> outputs;
    RelayPromise(stx::int_sequence_for<Es...>, RelayPromise*& promise) { promise = this; }
    auto handle() { return stdx::coroutine_handle<RelayPromise>::from_promise(*this); }
    RelayPromise* get_return_object() { return this; }
    void startRelay(Executor* exe) { RelayPromiseBase::startRelay(exe, handle()); }
    void reschedule() { RelayPromiseBase::reschedule(handle()); }

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

    int_t tick() const {
        return currentTimeline()->tick();
    }

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

    static RelayPromise* make() {
        return makeImpl(stx::int_sequence_for<Es...>());
    }

private:
    template<int_t... I>
    static RelayPromise* makeImpl(stx::int_sequence<I...>, RelayPromise* promise = {}) {
        while (true) {
            (promise->template sendEvents<I>(), ...);
            promise->reschedule();
            co_await stdx::suspend_always();
        }
    }
};

namespace detail {
    template<typename... Es, typename TL, int_t... I>
    Script recordEvents(RelayPromise<Es...>* output, AsyncTimeline<TL>* timeline, stx::Tuple<const SourceImpl<Es>*...> sources, stx::int_sequence<I...>) {
        while (true) {
            stx::Tuple<Receiver<Es>...> receivers {};
            await { ((*stx::get<I>(sources) > stx::get<I>(receivers)) || ...) };
            
            int_t tick = timeline->tick();
            ((output->template buffer<I>()[tick] = std::move(stx::get<I>(receivers))), ...);
        }
    }
}

template<typename... Ts>
struct Pack {
    static constexpr int_t size = sizeof...(Ts);
};
template<typename P1, typename P2>
struct pack_cat;
template<typename... Ts1, typename... Ts2>
struct pack_cat<Pack<Ts1...>, Pack<Ts2...>> {
    using type = Pack<Ts1..., Ts2...>;
};
template<typename P1, typename P2>
using pack_cat_t = typename pack_cat<P1, P2>::type;

template<template<typename> typename Check, typename P>
struct pack_select;
template<template<typename> typename Check, typename T, typename... Ts>
struct pack_select<Check, Pack<T, Ts...>> {
    using base_type = typename pack_select<Check, Pack<Ts...>>::type;
    using type = std::conditional_t<Check<T>::value,
        pack_cat_t<Pack<T>, base_type>, base_type>;
};
template<template<typename> typename Check>
struct pack_select<Check, Pack<>> {
    using type = Pack<>;
};
template<template<typename> typename Check, typename P>
using pack_select_t = typename pack_select<Check, P>::type;

template<template<typename...> typename Template, typename P>
struct with_pack;
template<template<typename...> typename Template, typename... Ts>
struct with_pack<Template, Pack<Ts...>> {
    using type = Template<Ts...>;
};
template<template<typename...> typename Template, typename P>
using with_pack_t = typename with_pack<Template, P>::type;

template<template<typename> typename Transform, typename>
struct transform_pack;
template<template<typename> typename Transform, typename... Ts>
struct transform_pack<Transform, Pack<Ts...>> {
    using type = Pack<Transform<Ts>...>;
};
template<template<typename> typename Transform, typename P>
using transform_pack_t = typename transform_pack<Transform, P>::type;

namespace detail {
    struct AsyncTimelineBase {
        int_t nextTickToSchedule = 0;
        void scheduleAsyncTick(Timeline*);
        int_t computeTick(const Timeline* timeline) const;
    };
}
template<std::derived_from<Timeline> TL>
class AsyncTimeline : detail::AsyncTimelineBase {
private:
    stx::Owned<stdx::coroutine_handle<>> m_relayPromise;
    TL m_timeline;
    stx::Owned<Script> eventRecorder;

    template<typename Arg>
    struct IsEventArg {
        static constexpr bool value = std::is_reference_v<Arg> && IsSource<std::remove_cvref_t<Arg>>;
    };
    template<typename S>
    using toEventType = typename std::remove_cvref_t<S>::event_type;
    template<typename... Args>
    using EventPack = pack_cat_t<Pack<CallbackEvent>, transform_pack_t<toEventType, pack_select_t<IsEventArg, Pack<Args...>>>>;
    template<typename... Args>
    using relay_promise = with_pack_t<RelayPromise, EventPack<Args...>>;
    template<typename... Args>
    auto* relayPromise() {
        return &stdx::coroutine_handle<relay_promise<Args...>>::from_address(m_relayPromise.address()).promise();
    }

    template<typename T, typename Promise>
    static decltype(auto) forwardOrRelay(const Promise* promise, std::remove_reference_t<T>& t) {
        if constexpr (IsSource<std::remove_cvref_t<T>>) {
            return promise->template output<typename std::remove_cvref_t<T>::event_type>();
        } else {
            return static_cast<T&&>(t);
        }
    }

    template<typename... Es>
    using SourceTuple = stx::Tuple<const SourceImpl<Es>*...>;
    template<typename Tuple, typename T>
    static void writeIfSource(Tuple& tuple, T& t) {
        if constexpr (IsSource<std::remove_cv_t<T>>) {
            stx::get<const SourceImpl<typename std::remove_cv_t<T>::event_type>*>(tuple) = &t;
        }
    }

protected:
    TL* timeline() { return &m_timeline; }
    const TL* timeline() const { return &m_timeline; }

public:
    template<typename... Args>
    AsyncTimeline(Args&&... args)
        : m_relayPromise(relay_promise<Args...>::make()->handle())
        , m_timeline(forwardOrRelay<Args>(relayPromise<Args...>(), args)...) {

        VERIFY(Event, evt::currentTimeline() == evt::mainTimeline());

        relayPromise<Args...>()->startRelay(timeline()->executor());

        with_pack_t<SourceTuple, EventPack<Args...>> sourceTuple {};
        writeIfSource(sourceTuple, callbackHandler());
        (writeIfSource(sourceTuple, args), ...);
        eventRecorder = detail::recordEvents(relayPromise<Args...>(), this, sourceTuple, stx::make_int_sequence<EventPack<Args...>::size>());
    }
    void asyncTick() {
        scheduleAsyncTick(timeline());
    }
    int_t tick() const { return computeTick(timeline()); }

    Source<CallbackEvent>& callbackHandler() { return timeline()->callbackHandler(); }
};

}
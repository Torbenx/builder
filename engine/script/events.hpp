#pragma once

#include "script/WaitExpression.hpp"
#include "util/coroutines.hpp"
#include <atomic>

template<LogicSet ls>
struct EventResult {
    const uint64_t events;

    EventResult(uint64_t events)
        : events(events) { }

    template<typename T, int_t id>
    bool operator()(SimpleWaitExpression<T, id>) {
        constexpr uint64_t mask = ls.bitmask(id);
        return events & mask;
    }
};

struct EventReceiver {
    const uint64_t key;
    std::atomic<uint64_t> events = 0;
    //ScheduleItem item;

    EventReceiver(uint64_t key)
        : key(key) { }

    void event(uint64_t mask) {
        uint64_t ev = events.fetch_or(mask, std::memory_order_relaxed);
        if (shoudSchedule(ev | mask) && !shoudSchedule(ev)) {
            // schedule it...
        }
    }

    bool shoudSchedule(uint64_t ev) { return (~((ev + key) ^ ev)) & key; }
};

template<LogicSet ls, typename... Holders>
struct EventAwaitable : EventReceiver {
    std::tuple<Holders...> tuple;
    std::array<int_t, sizeof...(Holders)> indices;

    EventAwaitable(uint64_t key, std::tuple<Holders...> tuple)
        : EventReceiver(key), tuple(tuple) { }

    bool await_ready() { return false; }

    template<size_t... I>
    void registerEvents(std::index_sequence<I...> seq) {
        registerEvents2<ls.bitmask(Holders::ID)...>(seq);
    }

    template<uint64_t... bitmasks, size_t... I>
    void registerEvents2(std::index_sequence<I...>) {
        ((indices[I] = std::get<I>(tuple).t.registerEvent(this, bitmasks)), ...);
    }

    template<size_t... I>
    void cancelEvents(std::index_sequence<I...>) {
        (std::get<I>(tuple).t.cancelEvent(this, indices[I]), ...);
    }

    void await_suspend(stdx::coroutine_handle<> handle) {
        registerEvents(std::index_sequence_for<Holders...>());
    }

    EventResult<ls> await_resume() {
        cancelEvents(std::index_sequence_for<Holders...>());
        return { events.load(std::memory_order_relaxed) };
    }
};

template<LogicSet ls, typename... Holders>
EventAwaitable<ls, Holders...> operator co_await(WaitExpression<ls, Holders...> expr) {
    constexpr uint64_t key = ls.key();
    return { key, expr.tuple };
}

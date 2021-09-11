#pragma once

#include "events/Scheduler.hpp"
#include "events/WaitExpression.hpp"
#include <atomic>

#define await co_await

namespace Tag {
struct Events {
    static constexpr char tag[] = "events";
    static constexpr bool verifyEnabled = true;
};
}

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
    ScheduleItem item;

    EventReceiver(uint64_t key)
        : key(key) { }

    void event(uint64_t mask) {
        uint64_t ev = events.fetch_or(mask, std::memory_order_relaxed);
        if (shoudSchedule(ev | mask) && !shoudSchedule(ev)) {
            scheduling::schedulingQueue.pushBack(&  item);
        }
    }

    bool shoudSchedule(uint64_t ev) { return (~((ev + key) ^ ev)) & key; }
};

template<LogicSet ls, typename... Holders>
struct EventAwaitable : EventReceiver {
    std::tuple<Holders...> tuple;

    EventAwaitable(uint64_t key, std::tuple<Holders...> tuple)
        : EventReceiver(key), tuple(tuple) { }

    bool await_ready() { return false; }

    template<size_t... I>
    void registerEvents(std::index_sequence<I...>) {
        //TODO: ensure ls.bitmask is evaluated a compile time
        (std::get<I>(tuple).t.registerEvent(this, ls.bitmask(std::get<I>(tuple).ID)), ...);
    }

    template<size_t... I>
    void cancelEvents(std::index_sequence<I...>) {
        (std::get<I>(tuple).t.cancelEvent(this), ...);
    }

    stdx::coroutine_handle<> await_suspend(stdx::coroutine_handle<> handle) {
        registerEvents(std::index_sequence_for<Holders...>());
        item.handle = handle;
        return scheduling::schedulingQueue.popFront()->handle;
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

class Script {
public:
    struct promise_type {
        Script get_return_object() { return { stdx::coroutine_handle<promise_type>::from_promise(*this) }; }

        auto initial_suspend() { return stdx::suspend_never(); }

        auto final_suspend() { return stdx::suspend_never(); }

        void unhandled_exception() {
            VERIFY_NOT_REACHED(Events);
            std::abort();
        }
    };

    stdx::coroutine_handle<promise_type> handle;
    void resume() {
        handle.resume();
    }
};

#pragma once

#include "Timeline.hpp"

namespace evt {

struct Defer {
    struct NodeType : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        void commit(Signal<> signal) const {
            signal.send();
        }
    };
    constexpr operator NodeType() const { return {}; }
};

struct NextTick {
    struct NodeType : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        void commit(Signal<> signal) {
            signal.script->executeOnNextTick = true;
            signal.send();
        }
        void cancel(Script script) {
            script->executeOnNextTick = false;
        }
    };
    constexpr operator NodeType() const { return {}; }
};

using MonoConditionNamed = NamedCondition<"monoCondition">;
struct MonoCondition : CompiledCondition<MonoConditionNamed::NodeType> {
    using Base = CompiledCondition<MonoConditionNamed::NodeType>;
    Signal<> signal(Script script) const { return Base::signal(script, MonoConditionNamed()); }
    constexpr MonoCondition() = default;
};
constexpr MonoCondition monoCondition;

struct BinaryState {
    bool state = false;
    mutable std::vector<Signal<>> signals;

    BinaryState() = default;
    BinaryState(bool state)
        : state(state) { }
    BinaryState(const BinaryState&) = delete;
    explicit operator bool() const { return state; }

    void set(bool newState) {
        if (newState == state)
            return;
        for (Signal<> signal : signals)
            signal.send((int_t)newState - (int_t)state);
        state = newState;
    }

    struct NodeType : TmplLeafNode {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        const BinaryState* binaryState = nullptr;
        uint64_t state() const {
            return binaryState->state;
        }
        void commit(Signal<> signal) const {
            binaryState->signals.push_back(signal);
        }
        void cancel(Script script) const {
            auto it = std::remove(binaryState->signals.begin(), binaryState->signals.end(), script);
            binaryState->signals.erase(it, binaryState->signals.end());
        }
    };
    constexpr operator NodeType() const { return { {}, this }; }
};

template<typename Callback>
struct SuspendAnd {
    Callback callback {};
    SuspendAnd() = default;
    SuspendAnd(const Callback& callback)
        : callback(callback) { }
    bool await_ready() { return false; }
    template<typename Promise>
    void await_suspend(stdx::coroutine_handle<Promise> handle) {
        callback(handle);
    }
    void await_resume() { }
};

inline auto onTimeline(Timeline* timeline) {
    return SuspendAnd([timeline](Script script) {
        timeline->executor()->schedule(script.handle);
    });
}

struct Ping : TmplLeafNode {
    static constexpr uint64_t requiredEvents() { return 1; }
    static constexpr uint64_t maxEvents() { return 1; }
    struct ScratchSpace {
        Signal<> signal;
        Source<CallbackEvent>* sourceHandler;
    };
    Source<CallbackEvent>& targetHandler;
    Ping(Source<CallbackEvent>& target)
        : targetHandler(target) { }
    void commit(Signal<>, ScratchSpace&) const;
    void cancel(Script, ScratchSpace&) const;
};

namespace detail {
    template<typename... Args>
    using RemoteInvTpl = stx::Tuple<Source<CallbackEvent>&, Script, Script (*)(Args...), Args...>;
    template<typename... Args, int_t... I>
    Script remoteRunInvoke(RemoteInvTpl<Args...>* tuple, stx::int_sequence<I...>) {
        Source<CallbackEvent>& sourceHandler = stx::get<0>(*tuple);
        Script sourceScript = stx::get<1>(*tuple);
        Script targetScript = stx::get<2>(*tuple)(std::forward<Args>(stx::get<I + 3>(*tuple))...);
        delete tuple;
        await { *targetScript->completion };
        postCallback(
            sourceHandler,
            {
                sourceScript.address(),
                [](void* address) {
                    Script::fromAddress(address).handle.destroy();
                },
            });
    }
}
template<typename... Args>
Script remoteRun(Source<CallbackEvent>& targetHandler, Script (*func)(Args...), Args... args) {
    co_await SuspendAnd([&](Script sourceScript) {
        Source<CallbackEvent>& sourceHandler = evt::currentTimeline()->callbackHandler();
        auto* tuple = new detail::RemoteInvTpl<Args...> { { { sourceHandler }, { sourceScript }, { func }, { std::forward<Args>(args) }... } };
        targetHandler.sendEvent({
            tuple,
            [](detail::RemoteInvTpl<Args...>* tuple) {
                detail::remoteRunInvoke(tuple, stx::int_sequence_for<Args...>());
            },
        });
    });
}

}
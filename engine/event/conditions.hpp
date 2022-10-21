#pragma once

#include "ScriptPromise.hpp"
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
    template<typename TL>
    Ping(AsyncTimeline<TL>& targetTimeline)
        : Ping(targetTimeline.callbackHandler()) { }
    void commit(Signal<>, ScratchSpace&) const;
    void cancel(Script, ScratchSpace&) const;
};

namespace detail {
    struct AsyncInvokeData {
        Source<CallbackEvent>& sourceHandler;
        Script sourceScript;
        Script (*func)(AsyncInvokeData*);
    };
    Script asyncInvoke(AsyncInvokeData* data);

    template<typename... Args>
    struct AsyncScriptData : AsyncInvokeData {
        Script (*func)(Args...);
        stx::Tuple<Args...> args;

        static Script invoke(AsyncInvokeData* data) {
            return invokeImpl((AsyncScriptData*)data, stx::int_sequence_for<Args...>());
        }
        template<int_t... I>
        static Script invokeImpl(AsyncScriptData* data, stx::int_sequence<I...>) {
            Script script = data->func(std::forward<Args>(stx::get<I>(data->args))...);
            delete data;
            return script;
        }
        AsyncScriptData(Source<CallbackEvent>& sourceHandler, Script sourceScript, Script (*func)(Args...), Args&&... args)
            : AsyncInvokeData { sourceHandler, sourceScript, invoke }
            , func(func)
            , args { { { std::forward<Args>(args) }... } } { }
    };
}
template<typename... Args>
Script asyncScript(Source<CallbackEvent>& targetHandler, Script (*func)(Args...), Args... args) {
    co_await SuspendAnd([&](Script sourceScript) {
        Source<CallbackEvent>& sourceHandler = evt::currentTimeline()->callbackHandler();
        detail::AsyncInvokeData* data = new detail::AsyncScriptData<Args...> {
            sourceHandler,
            sourceScript,
            func,
            std::forward<Args>(args)...
        };
        targetHandler.sendEvent({ data, stx::Call<detail::asyncInvoke>() });
    });
}
template<typename TL, typename... Args>
Script asyncScript(AsyncTimeline<TL>& targetTimeline, Script (*func)(Args...), Args... args) {
    return asyncScript(targetTimeline.callbackHandler(), func, std::forward<Args>(args)...);
}

}
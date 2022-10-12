#pragma once

#include "ScriptPromise.hpp"

namespace evt {

struct OnExecutor {
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        Executor* executor = nullptr;
        void commit(Signal signal) const {
            signal.script->executor = executor;
            signal.send();
        }
    };
    Executor* executor = nullptr;
    OnExecutor() = default;
    OnExecutor(Executor* executor)
        : executor(executor) { }
    constexpr operator NodeType() const { return { {}, executor }; }
};

struct NextTick {
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        ExecutorImpl* executor = nullptr;
        void commit(Signal signal) {
            executor = static_cast<ExecutorImpl*>(signal.script->executor);
            if (!executor)
                executor = static_cast<ExecutorImpl*>(evt::currentExecutor());
            signal.script->executor = &executor->nextTick;
            signal.send();
        }
        void cancel(ScriptHandle script) {
            VERIFY(Event, script->executor == &executor->nextTick);
            script->executor = executor;
        }
    };
    constexpr operator NodeType() const { return {}; }
};

using MonoConditionNamed = NamedCondition<"monoCondition">;
struct MonoCondition : CompiledCondition<MonoConditionNamed::NodeType> {
    using Base = CompiledCondition<MonoConditionNamed::NodeType>;
    Signal signal(ScriptHandle script) const { return Base::signal(script, MonoConditionNamed()); }
    constexpr MonoCondition() = default;
};
constexpr MonoCondition monoCondition;

struct BinaryState {
    bool state = false;
    mutable std::vector<Signal> signals;

    BinaryState() = default;
    BinaryState(bool state)
        : state(state) { }
    BinaryState(const BinaryState&) = delete;
    explicit operator bool() const { return state; }

    void set(bool newState) {
        if (newState == state)
            return;
        for (Signal signal : signals)
            signal.send((int_t)newState - (int_t)state);
        state = newState;
    }

    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        const BinaryState* binaryState = nullptr;
        uint64_t state() const {
            return binaryState->state;
        }
        void commit(Signal signal) const {
            binaryState->signals.push_back(signal);
        }
        void cancel(ScriptHandle script) const {
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

}
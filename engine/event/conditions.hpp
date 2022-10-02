#pragma once

#include "CompiledCondition.hpp"

namespace evt {

struct OnExecutor {
    struct NodeType : TmplLeafNode<NodeType> {
        Executor* executor = nullptr;
        void commit(Signal signal) const {
            signal.script->executor = executor;
        }
    };
    Executor* executor = nullptr;
    OnExecutor() = default;
    OnExecutor(Executor* executor)
        : executor(executor) { }
    constexpr operator NodeType() const { return { {}, executor }; }
};

struct Defer {
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        static void commit(Signal signal) {
            signal.send();
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
    struct NodeType : TmplLeafNode<NodeType> {
        static constexpr uint64_t requiredEvents() { return 1; }
        static constexpr uint64_t maxEvents() { return 1; }
        const BinaryState* binaryState = nullptr;
        uint64_t state() const {
            return binaryState->state;
        }
        void commit(Signal) const {
        }
        void cancel(ScriptHandle) const {
        }
    };
    constexpr operator NodeType() const { return { {}, this }; }
};

}
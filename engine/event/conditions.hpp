#pragma once

#include "CompiledCondition.hpp"

namespace evt {

struct OnExecutor {
    struct NodeType : TmplLeafNode<NodeType, Executor> {
        static void doCommit(Executor* exe, Signal signal) {
            signal.script->executor = exe;
        }
    };
    Executor* executor = nullptr;
    OnExecutor() = default;
    OnExecutor(Executor* executor)
        : executor(executor) { }
    constexpr operator NodeType() const { return { executor }; }
};

struct Defer {
    struct NodeType : TmplLeafNode<NodeType, void> {
        static constexpr uint64_t eventCount() { return 1; }
        static uint64_t getState(void*) { return 0; }
        static void doCommit(void*, Signal signal) {
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
    struct NodeType : TmplLeafNode<NodeType, const BinaryState> {
        static constexpr uint64_t eventCount() { return 1; }
        static uint64_t getState(const BinaryState* state) {
            return state->state;
        }
        static void doCommit(const BinaryState*, Signal) {
        }
        static void doCancel(const BinaryState*, ScriptHandle) {
        }
    };
    constexpr operator NodeType() const { return { this }; }
};

}
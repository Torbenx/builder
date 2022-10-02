#pragma once

#include "CompiledCondition.hpp"
#include "containers/Tuple.hpp"

namespace evt {

template<TmplTreeNode Node>
struct Awaitable {
    static constexpr const auto& output = CompiledCondition<Node>::output;
    template<int_t index>
    using Handler = NodeTypeOf<Node, output.leafs[index].tmplLoc>;

    struct LeafState {
        void* pointer = nullptr;
        uint64_t mask = 0;
    };
    struct LeafsWrapper {
        stx::Array<LeafState, output.leafs.size()> leafs;
        constexpr LeafState& operator[](int_t index) { return leafs[index]; }
        constexpr LeafsWrapper(const Node& node)
            : LeafsWrapper(node, stx::make_int_sequence<output.leafs.size()>()) { }
        template<int_t... I>
        constexpr LeafsWrapper(const Node& node, stx::int_sequence<I...>)
            : leafs { { { node.template get<output.leafs[I].tmplLoc>().pointer(), output.leafs[I].mask }... } } { }
    };
    LeafsWrapper leafs;

    uint64_t startMask = output.startMask;

    ScriptHandle suspendedScript = {};

    template<int_t... I>
    constexpr void applyState(stx::int_sequence<I...>) {
        startMask += (... + (Handler<I>::state(leafs[I].pointer) * leafs[I].mask));
    }
    constexpr bool await_ready() {
        applyState(stx::make_int_sequence<output.leafs.size()>());
        return (startMask & output.keyMask) != 0;
    }

    template<int_t... I>
    constexpr void commit(ScriptHandle script, stx::int_sequence<I...>) {
        (Handler<I>::commit(leafs[I].pointer, Signal { script, leafs[I].mask }), ...);
    }
    constexpr void await_suspend(ScriptHandle script) {
        suspendedScript = script;
        script->state = { startMask, output.keyMask };
        commit(script, stx::make_int_sequence<output.leafs.size()>());
    }

    constexpr void await_resume() { }

    template<int_t... I>
    constexpr void cancel(ScriptHandle script, stx::int_sequence<I...>) {
        (Handler<I>::cancel(leafs[I].pointer, script), ...);
    }
    constexpr ~Awaitable() {
        if (!suspendedScript)
            return;
        cancel(suspendedScript, stx::make_int_sequence<output.leafs.size()>());
    }
};

}

template<evt::TmplTreeNode Node>
inline evt::Awaitable<evt::converted_tree_node_t<Node>> operator co_await(const Node& node) {
    return { convertTreeNode(node) };
}
template<typename Node>
inline evt::Awaitable<Node> operator co_await(const evt::CompiledCondition<Node>& cond) {
    return { cond.node };
}
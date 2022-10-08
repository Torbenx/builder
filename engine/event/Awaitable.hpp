#pragma once

#include "CompiledCondition.hpp"
#include "containers/Tuple.hpp"

namespace evt {

template<TmplTreeNode Node>
struct Awaitable {
    static constexpr const auto& output = CompiledCondition<Node>::output;

    template<typename L>
    struct LeafWrapper {
        L leaf;
        uint64_t mask = 0;
    };
    template<typename L>
    LeafWrapper(const L&, uint64_t) -> LeafWrapper<L>;
    template<typename>
    struct LeafsTupleHelper;
    template<int_t... I>
    struct LeafsTupleHelper<stx::int_sequence<I...>> {
        using type = stx::Tuple<LeafWrapper<NodeTypeOf<Node, output.leafs[I].tmplLoc>>...>;
    };
    using LeafsTuple = typename LeafsTupleHelper<stx::make_int_sequence<output.leafs.size()>>::type;
    LeafsTuple leafs;

    uint64_t startMask = output.startMask;

    ScriptHandle suspendedScript = {};

    constexpr Awaitable(const Node& node)
        : Awaitable(node, stx::make_int_sequence<output.leafs.size()>()) { }
    template<int_t... I>
    constexpr Awaitable(const Node& node, stx::int_sequence<I...>)
        : leafs { { { makeWrapper<I>(node) }... } } { }
    template<int_t i>
    constexpr auto makeWrapper(const Node& node) {
        const auto& leaf = output.leafs[i];
        return LeafWrapper { node.template get<leaf.tmplLoc>(), leaf.mask };
    }
    constexpr bool await_ready() {
        stx::forEachElement(leafs, [&](const auto& wrapper) {
            startMask += wrapper.leaf.state() * wrapper.mask;
        });
        return (startMask & output.keyMask) != 0;
    }
    constexpr void await_suspend(ScriptHandle script) {
        suspendedScript = script;
        script->state = { startMask, output.keyMask };
        stx::forEachElement(leafs, [&](auto& wrapper) {
            wrapper.leaf.commit(Signal { script, wrapper.mask });
        });
    }
    constexpr void cancel() {
        stx::forEachElement(leafs, [&](auto& wrapper) {
            wrapper.leaf.cancel(suspendedScript);
        });
    }
    constexpr void await_resume() {
        if (suspendedScript)
            cancel();
        suspendedScript = {};
    }
    constexpr ~Awaitable() {
        if (suspendedScript)
            cancel();
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
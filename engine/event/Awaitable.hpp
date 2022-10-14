#pragma once

#include "ScriptPromise.hpp"
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
    template<int_t i>
    static constexpr auto makeWrapper(const Node& node) {
        const auto& leaf = output.leafs[i];
        return LeafWrapper { node.template get<leaf.tmplLoc>(), leaf.mask };
    }
    template<typename>
    struct LeafsTupleHelper;
    template<int_t... I>
    struct LeafsTupleHelper<stx::int_sequence<I...>> {
        using type = stx::Tuple<LeafWrapper<NodeTypeOf<Node, output.leafs[I].tmplLoc>>...>;
    };
    using LeafsTuple = typename LeafsTupleHelper<stx::make_int_sequence<output.leafs.size()>>::type;
    struct Leafs : LeafsTuple {
        constexpr Leafs(const CompiledCondition<Node>& condition)
            : Leafs(condition.node) { }
        template<std::convertible_to<Node> Node2>
        constexpr Leafs(const Node2& node)
            : Leafs(node, stx::make_int_sequence<output.leafs.size()>()) { }
        template<int_t... I>
        constexpr Leafs(const Node& node, stx::int_sequence<I...>)
            : LeafsTuple { { { makeWrapper<I>(node) }... } } { }
    };
    Leafs leafs;

    uint64_t startMask = output.startMask;

    Script suspendedScript = {};

    constexpr bool await_ready() {
        stx::forEachElement(leafs, [&](const auto& wrapper) {
            startMask += wrapper.leaf.state() * wrapper.mask;
        });
        return (startMask & output.keyMask) != 0;
    }
    constexpr void await_suspend(Script script) {
        suspendedScript = script;
        script->state = { startMask, output.keyMask };
        stx::forEachElement(leafs, [&](auto& wrapper) {
            wrapper.leaf.commit(Signal<> { script, wrapper.mask });
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
template<TmplTreeNode Node>
Awaitable(const Node&) -> Awaitable<converted_tree_node_t<Node>>;
template<typename Node>
Awaitable(const CompiledCondition<Node>&) -> Awaitable<Node>;

}

#define await co_await ::evt::Awaitable
#pragma once

#include "ScriptPromise.hpp"
#include "containers/Tuple.hpp"

namespace evt {

template<typename Node>
concept NeedsScratchSpace = requires { typename Node::ScratchSpace; };
template<typename Node>
struct scratch_space_or_empty {
    using type = struct { };
};
template<NeedsScratchSpace Node>
struct scratch_space_or_empty<Node> {
    using type = typename Node::ScratchSpace;
};

template<TmplTreeNode Node>
struct Awaitable {
    static constexpr const auto& output = CompiledCondition<Node>::output;
    static constexpr int_t LEAF_COUNT = output.leafs.size();

    template<int_t i>
    using NodeOf = typename CompiledCondition<Node>::template NodeOf<i>;

    template<typename L>
    struct LeafWrapper {
        using LeafType = L;
        L leaf;
        uint64_t mask = 0;
    };
    template<typename L>
    LeafWrapper(const L&, uint64_t) -> LeafWrapper<L>;
    template<int_t i>
    static constexpr auto makeWrapper(const Node& node) {
        const auto& leaf = output.leafs[i];
        return LeafWrapper { get<leaf.tmplLoc>(node), leaf.mask };
    }
    template<typename>
    struct LeafsTupleHelper;
    template<int_t... I>
    struct LeafsTupleHelper<stx::int_sequence<I...>> {
        using type = stx::Tuple<LeafWrapper<NodeOf<I>>...>;
    };
    using LeafsTuple = typename LeafsTupleHelper<stx::make_int_sequence<LEAF_COUNT>>::type;
    struct Leafs : LeafsTuple {
        constexpr Leafs(const CompiledCondition<Node>& condition)
            : Leafs(condition.node) { }
        template<std::convertible_to<Node> Node2>
        constexpr Leafs(const Node2& node)
            : Leafs(node, stx::make_int_sequence<LEAF_COUNT>()) { }
        template<int_t... I>
        constexpr Leafs(const Node& node, stx::int_sequence<I...>)
            : LeafsTuple { { { makeWrapper<I>(node) }... } } { }
    };
    Leafs leafs;

    uint64_t startMask = output.startMask;

    Script suspendedScript = {};

    template<typename>
    struct ScratchTupleHelper;
    template<int_t... I>
    struct ScratchTupleHelper<stx::int_sequence<I...>> {
        using type = stx::Tuple<typename scratch_space_or_empty<NodeOf<I>>::type...>;
    };
    using ScratchTuple = typename ScratchTupleHelper<stx::make_int_sequence<LEAF_COUNT>>::type;
    ScratchTuple&& scratchSpaces = {};

    constexpr bool await_ready() {
        stx::forEachElement(leafs, [&](const auto& wrapper) {
            startMask += wrapper.leaf.state() * wrapper.mask;
        });
        return (startMask & output.keyMask) != 0;
    }
    constexpr void await_suspend(Script script) {
        suspendedScript = script;
        script->state = { startMask, output.keyMask };
        stx::forEachElementIndexed(leafs, [&]<typename Wrapper, int_t i>(Wrapper& wrapper, std::integral_constant<int_t, i>) {
            if constexpr (NeedsScratchSpace<typename Wrapper::LeafType>) {
                wrapper.leaf.commit(Signal<> { script, wrapper.mask }, stx::get<i>(scratchSpaces));
            } else {
                wrapper.leaf.commit(Signal<> { script, wrapper.mask });
            }
        });
    }
    constexpr void cancel() {
        stx::forEachElementIndexed(leafs, [&]<typename Wrapper, int_t i>(Wrapper& wrapper, std::integral_constant<int_t, i>) {
            if constexpr (NeedsScratchSpace<typename Wrapper::LeafType>) {
                wrapper.leaf.cancel(suspendedScript, stx::get<i>(scratchSpaces));
            } else {
                wrapper.leaf.cancel(suspendedScript);
            }
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
#pragma once

#include "Awaitable.hpp"

namespace evt {

struct CompiledEvent {
    struct NodeType {
        static constexpr uint64_t eventCount() { return 1; }
        const CompiledEvent* event = nullptr;
        template<TmplLeafLoc loc>
        constexpr const NodeType& get() const {
            static_assert(loc == TmplLeafLoc {});
            return *this;
        }
    };
    consteval CompiledEvent() {};
    CompiledEvent(const CompiledEvent&) = delete;
    constexpr operator NodeType() const { return { this }; }
};

template<typename Node>
struct CompiledCondition {
    static constexpr auto output = computeAwaitable<Node>();

    struct LeafInfo {
        const CompiledEvent* event = nullptr;
        uint64_t mask = 0;
    };
    stx::Array<LeafInfo, output.leafs.size()> leafs;

    static consteval AwaitableState initialState() {
        return { output.startMask, output.keyMask };
    }

    consteval CompiledCondition(const Node& node)
        : CompiledCondition(node, stx::make_int_sequence<output.leafs.size()>()) { }
    template<int_t i>
    consteval LeafInfo makeInfo(const Node& node) {
        const auto& leaf = node.template get<output.leafs[i].tmplLoc>();
        static_assert(std::is_same_v<std::remove_cvref_t<decltype(leaf)>, CompiledEvent::NodeType>);
        return { leaf.event, output.leafs[i].mask };
    }
    template<int_t... I>
    consteval CompiledCondition(const Node& node, stx::int_sequence<I...>)
        : leafs { { makeInfo<I>(node)... } } { }

    uint64_t makeMask(const CompiledEvent& event) const {
        uint64_t mask = 0;
        for (LeafInfo info : leafs) {
            if (info.event == &event)
                mask += info.mask;
        }
        return mask;
    }
    template<const CompiledEvent& event>
    Signal signal(ScriptHandle script) const {
        return { script, makeMask(event) };
    }
};
template<TmplTreeNode Node>
CompiledCondition(const Node&) -> CompiledCondition<typename Node::NodeType>;

template<typename Condition>
struct CompiledAwaitable {
    bool await_ready() { return false; }
    void await_suspend(ScriptHandle handle) {
        handle->state = Condition::initialState();
    }
    void await_resume() { }
};

struct Anything {
    static constexpr CompiledEvent EVENT {};
    static constexpr CompiledCondition CONDITION { EVENT };
    using Condition = decltype(CONDITION);
    static Signal signal(ScriptHandle script) { return CONDITION.signal<EVENT>(script); }
    constexpr CompiledAwaitable<Condition> operator co_await() const { return {}; }
};

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

inline Script test_awaitable() {
    using namespace code;
    VDUMP(Anything::CONDITION.initialState().state, ":#b");
    VDUMP(Anything::CONDITION.initialState().key, ":#b");
    VDUMP(Anything::CONDITION.makeMask(Anything::EVENT), ":#b");

    BinaryState b1, b2, b3;
    Awaitable await { (b1 && b2 || b3) && OnExecutor(nullptr) };
    VDUMP(await.output.startMask, ":#b");
    VDUMP(await.output.keyMask, ":#b");

    co_return;
}

}
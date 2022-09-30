#pragma once

#include "containers/Array.hpp"
#include "containers/Tuple.hpp"
#include "util/types.hpp"
#include <algorithm>
#include <variant>
#include <vector>

namespace evt {

enum class LogicalOp {
    And,
    Or,
};

template<typename T>
concept TmplTreeNode = requires {
    typename T::NodeType;
    std::convertible_to<T, typename T::NodeType>;
};

struct TmplLeafLoc {
    using mask_t = uint64_t;
    mask_t location = 0;
    constexpr TmplLeafLoc operator<<(int_t shift) const {
        return { location << shift };
    }
    constexpr TmplLeafLoc operator>>(int_t shift) const {
        return { location >> shift };
    }
    constexpr bool lowestBit() const { return location & (mask_t)1; }
    constexpr TmplLeafLoc withLowestBit() const { return { location | (mask_t)1 }; }
    constexpr bool operator==(const TmplLeafLoc&) const = default;
};

template<LogicalOp op, typename Left, typename Right>
struct TmplBranchNode {
    using NodeType = TmplBranchNode;
    Left left;
    Right right;
    template<TmplLeafLoc loc>
    const auto& get() const {
        if constexpr (loc.lowestBit())
            return right.template get<(loc >> 1)>();
        else
            return left.template get<(loc >> 1)>();
    }
};

template<typename Impl, typename pointer_type>
struct TmplLeafNode {
    using NodeType = Impl;
    pointer_type* m_pointer = nullptr;
    constexpr TmplLeafNode() = default;
    constexpr TmplLeafNode(pointer_type* p)
        : m_pointer(p) { }

    template<TmplLeafLoc loc>
    const Impl& get() const {
        static_assert(loc == TmplLeafLoc {});
        return *static_cast<const Impl*>(this);
    }
    void* pointer() const { return (void*)m_pointer; }

    static uint64_t getState(pointer_type*) { return 0; }
    static uint64_t state(void* data) {
        return Impl::getState(static_cast<pointer_type*>(data));
    }
    static constexpr uint64_t eventCount() { return 0; }

    static void doCommit(pointer_type*, Signal) { }
    static void commit(void* data, Signal sig) {
        Impl::doCommit(static_cast<pointer_type*>(data), sig);
    }

    static void doCancel(pointer_type*, ScriptHandle) { }
    static void cancel(void* data, ScriptHandle scr) {
        Impl::doCancel(static_cast<pointer_type*>(data), scr);
    }
};

namespace computation {

    struct LeafNode {
        TmplLeafLoc tmplLoc;
        uint64_t eventCount = 0;
        constexpr bool operator==(const LeafNode&) const = default;
    };

    struct TmplLeafLocBuilder {
        using mask_t = TmplLeafLoc::mask_t;
        mask_t location = 0;
        int_t insertLocation = 0;
        constexpr TmplLeafLocBuilder goLeft() const {
            return { location, insertLocation + 1 };
        }
        constexpr TmplLeafLocBuilder goRight() const {
            return { location | ((mask_t)1 << insertLocation), insertLocation + 1 };
        }
        constexpr TmplLeafLoc build() const {
            return { location };
        }
    };

    struct AndNode {
        std::vector<LeafNode> allOf;
        struct {
            std::vector<LeafNode> nodes;
            int_t target = 0;
            int_t upperBound = 0; // inclusive
        } atLeast;
    };
    struct CanonicalTree {
        std::vector<AndNode> anyOf;
    };

    constexpr void deduplicate(std::vector<LeafNode>& leafs) {
        for (int_t i = 0; i < (int_t)leafs.size(); i++) {
            auto it = leafs.begin() + i;
            auto newEnd = std::remove(it + 1, leafs.end(), *it);
            leafs.erase(newEnd, leafs.end());
        }
    }
    template<typename Node>
    constexpr CanonicalTree buildFromTmpl(std::type_identity<Node>, TmplLeafLocBuilder loc) {
        CanonicalTree out;
        AndNode andNode;
        andNode.allOf.push_back(LeafNode { loc.build(), Node::eventCount() });
        out.anyOf.push_back(std::move(andNode));
        return out;
    }
    template<LogicalOp op, typename Left, typename Right>
    constexpr CanonicalTree buildFromTmpl(std::type_identity<TmplBranchNode<op, Left, Right>>, TmplLeafLocBuilder loc) {
        CanonicalTree left = buildFromTmpl(std::type_identity<Left>(), loc.goLeft());
        CanonicalTree right = buildFromTmpl(std::type_identity<Right>(), loc.goRight());
        CanonicalTree out;
        if (op == LogicalOp::Or) {
            out.anyOf.insert(out.anyOf.end(), left.anyOf.begin(), left.anyOf.end());
            out.anyOf.insert(out.anyOf.end(), right.anyOf.begin(), right.anyOf.end());
        } else {
            for (auto& leftNode : left.anyOf) {
                for (auto& rightNode : right.anyOf) {
                    AndNode andNode;
                    andNode.allOf.insert(andNode.allOf.end(), leftNode.allOf.begin(), leftNode.allOf.end());
                    andNode.allOf.insert(andNode.allOf.end(), rightNode.allOf.begin(), rightNode.allOf.end());
                    deduplicate(andNode.allOf);
                    out.anyOf.emplace_back(std::move(andNode));
                }
            }
        }
        return out;
    }

    struct AndNodeInfo {
        uint64_t baseCount = 0;
        int_t usedBits = 0;
        uint64_t allOfValue = 1;
        int_t offset = 0;
    };
    constexpr AndNodeInfo processAndNode(const AndNode& in) {
        AndNodeInfo out;
        out.allOfValue = in.atLeast.upperBound + 1;
        uint64_t eventCount = 0;
        for (LeafNode leaf : in.allOf)
            eventCount += leaf.eventCount;
        uint64_t flipCount = out.allOfValue * eventCount + in.atLeast.target;
        uint64_t upperBound = out.allOfValue * eventCount + in.atLeast.upperBound;
        uint64_t upperBoundCeil = std::bit_ceil(upperBound);
        out.baseCount = upperBoundCeil - flipCount;
        out.usedBits = std::countr_zero(upperBoundCeil);
        return out;
    }
    constexpr bool allocateSpace(ArrayView<AndNodeInfo> infos) {
        int_t offset = 0;
        for (auto& info : infos) {
            info.offset = offset;
            offset += info.usedBits + 1;
        }
        return offset <= 64;
    }

    struct LeafNodeInfo {
        TmplLeafLoc tmplLoc;
        uint64_t mask = 0;
        constexpr bool operator==(const TmplLeafLoc& loc) const { return tmplLoc == loc; }
    };
    constexpr std::vector<LeafNodeInfo> accumulateMasks(ArrayView<const AndNode> nodes, ArrayView<const AndNodeInfo> infos) {
        VERIFY(Event, nodes.size() == infos.size());
        std::vector<LeafNodeInfo> out;
        auto findOrInsert = [&](const LeafNode& node) -> LeafNodeInfo& {
            auto it = std::find(out.begin(), out.end(), node.tmplLoc);
            if (it != out.end())
                return *it;
            out.push_back({ .tmplLoc = node.tmplLoc });
            return out.back();
        };
        for (int_t i = 0; i < nodes.size(); i++) {
            const AndNode& node = nodes[i];
            const AndNodeInfo& info = infos[i];
            for (const LeafNode& leaf : node.allOf)
                findOrInsert(leaf).mask += info.allOfValue << info.offset;
            for (const LeafNode& leaf : node.atLeast.nodes)
                findOrInsert(leaf).mask += (uint64_t)1 << info.offset;
        }
        return out;
    }

    constexpr uint64_t buildStartMask(ArrayView<const AndNodeInfo> infos) {
        uint64_t mask = 0;
        for (const AndNodeInfo& info : infos)
            mask += info.baseCount << info.offset;
        return mask;
    }
    constexpr uint64_t buildKeyMask(ArrayView<const AndNodeInfo> infos) {
        uint64_t mask = 0;
        for (const AndNodeInfo& info : infos)
            mask |= (uint64_t)1 << (info.offset + info.usedBits);
        return mask;
    }

    struct Output {
        uint64_t startMask = 0;
        uint64_t keyMask = 0;
        std::vector<LeafNodeInfo> leafs;
        std::vector<int_t> returnObjects;
    };

    template<typename Node>
    constexpr Output perform() {
        CanonicalTree tree = buildFromTmpl(std::type_identity<Node>(), {});
        std::vector<AndNodeInfo> andInfos;
        for (auto& node : tree.anyOf)
            andInfos.push_back(processAndNode(node));
        bool ret = allocateSpace(andInfos);
        VERIFY(Event, ret);
        std::vector<LeafNodeInfo> leafInfos = accumulateMasks(tree.anyOf, andInfos);
        return {
            .startMask = buildStartMask(andInfos),
            .keyMask = buildKeyMask(andInfos),
            .leafs = leafInfos,
            .returnObjects = {},
        };
    }

    template<typename T>
    struct OversizedStorage {
        T storage[1000] = {};
        int_t m_size = 0;
        constexpr OversizedStorage(ArrayView<const T> in)
            : m_size(in.size()) {
            std::copy_n(in.data(), in.size(), storage);
        }
        constexpr const T* data() const { return storage; }
        constexpr int_t size() const { return m_size; }
    };
    template<typename T, OversizedStorage<T> oversized>
    constexpr stx::Array<T, oversized.size()> rightsized() {
        stx::Array<T, oversized.size()> out;
        std::copy_n(oversized.data(), oversized.size(), out.data());
        return out;
    }
    template<typename T, OversizedStorage<T> oversized>
    struct StaticArrayView : ArrayView<const T> {
        static constexpr auto storage = rightsized<T, oversized>();
        constexpr StaticArrayView()
            : ArrayView<const T>(storage) { }
    };

    struct OversizedOutput {
        uint64_t startMask = 0;
        uint64_t keyMask = 0;
        OversizedStorage<LeafNodeInfo> leafs;
        OversizedStorage<int_t> returnObjects;
        constexpr OversizedOutput(const Output& in)
            : startMask(in.startMask)
            , keyMask(in.keyMask)
            , leafs(in.leafs)
            , returnObjects(in.returnObjects) { }
    };
    struct OutputView {
        uint64_t startMask = 0;
        uint64_t keyMask = 0;
        ArrayView<const LeafNodeInfo> leafs;
        ArrayView<const int_t> returnObjects;
    };
    template<OversizedOutput oversized>
    struct StaticOutputView : OutputView {
        constexpr StaticOutputView()
            : OutputView {
                .startMask = oversized.startMask,
                .keyMask = oversized.keyMask,
                .leafs = StaticArrayView<LeafNodeInfo, oversized.leafs>(),
                .returnObjects = StaticArrayView<int_t, oversized.returnObjects>(),
            } { }
    };
}

template<typename Node>
constexpr computation::OutputView computeAwaitable() {
    return computation::StaticOutputView<computation::perform<Node>()>();
}

template<typename Node>
struct Awaitable {
    static constexpr auto output = computeAwaitable<Node>();
    template<int_t index>
    using Handler = std::remove_cvref_t<decltype(std::declval<Node>().template get<output.leafs[index].tmplLoc>())>;

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

    template<typename T>
    struct ReturnObjectHelper;
    template<int_t... I>
    struct ReturnObjectHelper<stx::int_sequence<I...>> {
        using type = stx::Tuple<typename Handler<output.returnObjects[I]>::ReturnObject...>;
    };
    using ReturnObject = ReturnObjectHelper<stx::make_int_sequence<output.returnObjects.size()>>;
    ReturnObject&& returnObject = {};

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
template<TmplTreeNode Node>
Awaitable(const Node&) -> Awaitable<typename Node::NodeType>;

template<TmplTreeNode Node>
Awaitable<typename Node::NodeType> operator co_await(const Node& node) {
    return Awaitable<typename Node::NodeType> { static_cast<typename Node::NodeType>(node) };
}

namespace code {
    template<TmplTreeNode Left, TmplTreeNode Right>
    constexpr TmplBranchNode<LogicalOp::And, typename Left::NodeType, typename Right::NodeType> operator&&(const Left& left, const Right& right) {
        return { static_cast<typename Left::NodeType>(left), static_cast<typename Right::NodeType>(right) };
    }
    template<TmplTreeNode Left, TmplTreeNode Right>
    constexpr TmplBranchNode<LogicalOp::Or, typename Left::NodeType, typename Right::NodeType> operator||(const Left& left, const Right& right) {
        return { static_cast<typename Left::NodeType>(left), static_cast<typename Right::NodeType>(right) };
    }
}

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
            signal.notify();
        }
    };
    constexpr operator NodeType() const { return {}; }
};

}
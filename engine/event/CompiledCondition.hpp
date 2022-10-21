#pragma once

#include "Script.hpp"
#include "containers/Array.hpp"
#include "containers/FixedString.hpp"
#include "util/types.hpp"
#include <algorithm>
#include <variant>
#include <vector>

namespace evt {

enum class LogicalOp {
    And,
    Or,
    Comma,
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
    Left left;
    Right right;
};
template<TmplLeafLoc loc, std::derived_from<TmplLeafNode> Node>
const Node& get(const Node& node) {
    static_assert(loc == TmplLeafLoc {});
    return node;
}
template<TmplLeafLoc loc, LogicalOp op, typename Left, typename Right>
const auto& get(const TmplBranchNode<op, Left, Right>& node) {
    if constexpr (loc.lowestBit())
        return get<(loc >> 1)>(node.right);
    else
        return get<(loc >> 1)>(node.left);
}

template<typename T>
struct is_tmpl_branch_node {
    static constexpr bool value = false;
};
template<LogicalOp op, typename Left, typename Right>
struct is_tmpl_branch_node<TmplBranchNode<op, Left, Right>> {
    static constexpr bool value = true;
};
template<typename Node>
concept BranchTreeNode = is_tmpl_branch_node<Node>::value;
template<typename Node>
concept LeafTreeNode = std::derived_from<Node, TmplLeafNode>;
template<typename Node>
concept ActualTreeNode = BranchTreeNode<Node> || LeafTreeNode<Node>;
template<typename Node>
concept ConvertibleToTreeNode = !ActualTreeNode<Node> && requires {
    typename Node::NodeType;
    requires std::convertible_to<Node, typename Node::NodeType>;
};
template<typename Node>
concept TmplTreeNode = ActualTreeNode<Node> || ConvertibleToTreeNode<Node>;

template<typename Node>
struct converted_tree_node;
template<ActualTreeNode Node>
struct converted_tree_node<Node> {
    using type = Node;
};
template<ConvertibleToTreeNode Node>
struct converted_tree_node<Node> {
    using type = typename Node::NodeType;
};
template<typename Node>
using converted_tree_node_t = typename converted_tree_node<Node>::type;
template<ActualTreeNode Node>
constexpr Node convertTreeNode(const Node& node) {
    return node;
}
template<ConvertibleToTreeNode Node>
constexpr typename Node::NodeType convertTreeNode(const Node& node) {
    return static_cast<typename Node::NodeType>(node);
}

template<typename Node, TmplLeafLoc loc>
using NodeTypeOf = std::remove_cvref_t<decltype(get<loc>(std::declval<Node>()))>;

namespace compiled {

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
            int_t upperBound = 0; // inclusive
        } atLeast;
    };
    struct CanonicalTree {
        std::vector<AndNode> anyOf;
    };

    struct LeafNodeInfo {
        TmplLeafLoc tmplLoc;
        uint64_t mask = 0;
        constexpr bool operator==(const TmplLeafLoc& loc) const { return tmplLoc == loc; }
    };

    constexpr void deduplicate(std::vector<LeafNode>& leafs) {
        for (int_t i = 0; i < (int_t)leafs.size(); i++) {
            auto it = leafs.begin() + i;
            auto newEnd = std::remove(it + 1, leafs.end(), *it);
            leafs.erase(newEnd, leafs.end());
        }
    }
    template<typename Node>
    constexpr CanonicalTree buildFromTmpl(std::type_identity<Node>, TmplLeafLocBuilder loc, std::vector<LeafNodeInfo>& allLeafs) {
        CanonicalTree out;
        AndNode andNode;
        VERIFY(Event, Node::maxEvents() >= Node::requiredEvents());
        if (Node::maxEvents() == Node::requiredEvents()) {
            andNode.allOf.push_back({ loc.build(), Node::requiredEvents() });
        } else {
            andNode.atLeast.nodes.push_back({ loc.build(), Node::requiredEvents() });
            andNode.atLeast.upperBound += Node::maxEvents();
        }
        out.anyOf.push_back(std::move(andNode));
        allLeafs.push_back({ loc.build() });
        return out;
    }
    constexpr AndNode mergeAndNodes(const AndNode& in1, const AndNode& in2) {
        AndNode out;
        out.allOf.insert(out.allOf.end(), in1.allOf.begin(), in1.allOf.end());
        out.allOf.insert(out.allOf.end(), in2.allOf.begin(), in2.allOf.end());
        deduplicate(out.allOf);
        VERIFY(Event, in1.atLeast.upperBound == 0 || in2.atLeast.upperBound == 0);
        if (in1.atLeast.upperBound > 0)
            out.atLeast = in1.atLeast;
        else if (in2.atLeast.upperBound > 0)
            out.atLeast = in2.atLeast;
        return out;
    }
    template<LogicalOp op, typename Left, typename Right>
    constexpr CanonicalTree buildFromTmpl(std::type_identity<TmplBranchNode<op, Left, Right>>, TmplLeafLocBuilder loc, std::vector<LeafNodeInfo>& allLeafs) {
        CanonicalTree left = buildFromTmpl(std::type_identity<Left>(), loc.goLeft(), allLeafs);
        CanonicalTree right = buildFromTmpl(std::type_identity<Right>(), loc.goRight(), allLeafs);
        CanonicalTree out;
        if (op == LogicalOp::Or) {
            out.anyOf.insert(out.anyOf.end(), left.anyOf.begin(), left.anyOf.end());
            out.anyOf.insert(out.anyOf.end(), right.anyOf.begin(), right.anyOf.end());
        } else if (op == LogicalOp::And) {
            for (auto& leftNode : left.anyOf) {
                for (auto& rightNode : right.anyOf) {
                    out.anyOf.emplace_back(mergeAndNodes(leftNode, rightNode));
                }
            }
        } else {
            out = left;
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
        uint64_t atLeastTarget = 0;
        for (LeafNode leaf : in.atLeast.nodes)
            atLeastTarget += leaf.eventCount;
        uint64_t flipCount = out.allOfValue * eventCount + atLeastTarget;
        uint64_t upperBound = out.allOfValue * eventCount + in.atLeast.upperBound;
        uint64_t upperBoundCeil = std::bit_ceil(upperBound);
        out.baseCount = upperBoundCeil - flipCount;
        out.usedBits = std::countr_zero(upperBoundCeil) + 1;
        return out;
    }
    constexpr bool allocateSpace(ArrayView<AndNodeInfo> infos) {
        int_t offset = 0;
        for (auto& info : infos) {
            info.offset = offset;
            offset += info.usedBits;
        }
        return offset <= 64;
    }

    constexpr void accumulateMasks(std::vector<LeafNodeInfo>& out, ArrayView<const AndNode> nodes, ArrayView<const AndNodeInfo> infos) {
        VERIFY(Event, nodes.size() == infos.size());
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
            mask |= (uint64_t)1 << (info.offset + info.usedBits - 1);
        return mask;
    }

    struct Output {
        uint64_t startMask = 0;
        uint64_t keyMask = 0;
        std::vector<LeafNodeInfo> leafs;
    };

    template<typename Node>
    constexpr Output compile() {
        std::vector<LeafNodeInfo> allLeafs;
        CanonicalTree tree = buildFromTmpl(std::type_identity<Node>(), {}, allLeafs);
        std::vector<AndNodeInfo> andInfos;
        for (auto& node : tree.anyOf)
            andInfos.push_back(processAndNode(node));
        bool ret = allocateSpace(andInfos);
        VERIFY(Event, ret);
        accumulateMasks(allLeafs, tree.anyOf, andInfos);
        return {
            .startMask = buildStartMask(andInfos),
            .keyMask = buildKeyMask(andInfos),
            .leafs = allLeafs,
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
        constexpr OversizedOutput(const Output& in)
            : startMask(in.startMask)
            , keyMask(in.keyMask)
            , leafs(in.leafs) { }
    };
    struct OutputView {
        uint64_t startMask = 0;
        uint64_t keyMask = 0;
        ArrayView<const LeafNodeInfo> leafs;
    };
    template<OversizedOutput oversized>
    struct StaticOutputView : OutputView {
        constexpr StaticOutputView()
            : OutputView {
                .startMask = oversized.startMask,
                .keyMask = oversized.keyMask,
                .leafs = StaticArrayView<LeafNodeInfo, oversized.leafs>(),
            } { }
    };

}

template<typename Node>
constexpr compiled::OutputView compileCondition() {
    return compiled::StaticOutputView<compiled::compile<Node>()>();
}

template<stx::FixedString str>
struct NamedCondition {
    static constexpr stx::FixedString name = str;

    struct NodeType : TmplLeafNode {
        static constexpr uint64_t maxEvents() { return UNBOUND_EVENT_COUNT; }
        static constexpr uint64_t requiredEvents() { return 1; }
    };
    constexpr operator NodeType() const { return {}; }

    constexpr operator std::string_view() const { return name; }
};

template<TmplTreeNode Node>
struct CompiledCondition {
    using RootNode = Node;
    static constexpr auto output = compileCondition<Node>();

    template<int_t i>
    using NodeOf = std::remove_cvref_t<decltype(get<output.leafs[i].tmplLoc>(std::declval<Node>()))>;

    template<stx::FixedString name>
    static constexpr uint64_t makeMask(NamedCondition<name> event) {
        return makeMask(event, stx::make_int_sequence<output.leafs.size()>());
    }
    template<stx::FixedString name, int_t... I>
    static constexpr uint64_t makeMask(NamedCondition<name> event, stx::int_sequence<I...>) {
        return (... + selectMask<I>(event));
    }
    template<int_t i, stx::FixedString name>
    static constexpr uint64_t selectMask(NamedCondition<name>) {
        if constexpr (std::is_base_of_v<typename NamedCondition<name>::NodeType, NodeOf<i>>)
            return output.leafs[i].mask;
        else
            return 0;
    }
    template<stx::FixedString name>
    Signal<> signal(Script script, NamedCondition<name> event) const {
        static constexpr uint64_t mask = makeMask(event);
        static_assert(mask != 0);
        return { script, mask };
    }

    Node node;
    constexpr CompiledCondition() = default;
    constexpr CompiledCondition(const Node& node)
        : node(node) { }
};
template<TmplTreeNode TreeNode>
CompiledCondition(const TreeNode&) -> CompiledCondition<typename TreeNode::NodeType>;

template<TmplTreeNode Base, uint64_t required, uint64_t upperBound>
struct AtLeast : converted_tree_node_t<Base> {
    static constexpr uint64_t requiredEvents() { return required; }
    static constexpr uint64_t maxEvents() { return upperBound; }
};
template<uint64_t required, uint64_t upperBound = UNBOUND_EVENT_COUNT, TmplTreeNode Base>
constexpr AtLeast<Base, required, std::min(upperBound, converted_tree_node_t<Base>::maxEvents())> atLeast(const Base& base) {
    return { { convertTreeNode(base) } };
}

template<TmplTreeNode Base, uint64_t upperBound>
struct UpperBound : converted_tree_node_t<Base> {
    static constexpr uint64_t maxEvents() { return upperBound; }
};
template<uint64_t ub, TmplTreeNode Base>
constexpr auto upperBound(const Base& base) {
    constexpr uint64_t newUB = std::min(ub, converted_tree_node_t<Base>::maxEvents());
    return UpperBound<Base, newUB> { { convertTreeNode(base) } };
}

}

template<evt::TmplTreeNode Left, evt::TmplTreeNode Right>
constexpr evt::TmplBranchNode<evt::LogicalOp::And, evt::converted_tree_node_t<Left>, evt::converted_tree_node_t<Right>> operator&&(const Left& left, const Right& right) {
    return { evt::convertTreeNode(left), evt::convertTreeNode(right) };
}
template<evt::TmplTreeNode Left, evt::TmplTreeNode Right>
constexpr evt::TmplBranchNode<evt::LogicalOp::Or, evt::converted_tree_node_t<Left>, evt::converted_tree_node_t<Right>> operator||(const Left& left, const Right& right) {
    return { evt::convertTreeNode(left), evt::convertTreeNode(right) };
}
template<evt::TmplTreeNode Left, evt::TmplTreeNode Right>
constexpr evt::TmplBranchNode<evt::LogicalOp::Comma, evt::converted_tree_node_t<Left>, evt::converted_tree_node_t<Right>> operator,(const Left& left, const Right& right) {
    return { evt::convertTreeNode(left), evt::convertTreeNode(right) };
}

template<stx::FixedString name>
constexpr evt::NamedCondition<name> operator"" _ev() { return {}; }

namespace evt {

using MonoConditionNamed = NamedCondition<"monoCondition">;
struct MonoCondition : CompiledCondition<MonoConditionNamed::NodeType> {
    using Base = CompiledCondition<MonoConditionNamed::NodeType>;
    Signal<> signal(Script script) const { return Base::signal(script, MonoConditionNamed()); }
    constexpr MonoCondition() = default;
};
constexpr MonoCondition monoCondition;

}

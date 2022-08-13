#pragma once

#include "containers/BitSet.hpp"
#include "containers/FlatSet.hpp"
#include "containers/IdAllocator.hpp"
#include "util/uninitialized.hpp"
#include <vector>

template<typename Node>
class DirectedAcyclicGraph {
private:
    using idx_t = uint32_t;

public:
    class NodeHandle {
    private:
        NodeHandle(size_t index)
            : index(static_cast<idx_t>(index)) { }
        idx_t index = 0;
        friend DirectedAcyclicGraph;
    };

private:
    struct NodeContainer {
        uninitialized<Node> node {};
        FlatSet<NodeHandle> parents {};
    };
    NodeContainer* nodes = nullptr;
    int_t nodeCapacity = 0;
    IdAllocator<idx_t> indexAllocator;

    void ensureSize(idx_t index) {
        if (nodeCapacity > index)
            return;
        nodeCapacity = nodeCapacity == 0 ? 4 : nodeCapacity * 2;
        NodeContainer* newNodes = new NodeContainer[nodeCapacity];
        forEachNode([&](NodeHandle handle) {
            if (handle.index == index)
                return;
            newNodes[handle.index].node.initialize(std::move(nodes[handle.index].node.value));
            nodes[handle.index].node.destroy();
            newNodes[handle.index].parents = std::move(nodes[handle.index].parents);
        });
        delete[] nodes;
        nodes = newNodes;
    }
    idx_t newIndex() {
        idx_t index = indexAllocator.allocateId();
        ensureSize(index);
        return index;
    }
    NodeContainer& at(NodeHandle handle) { return nodes[handle.index]; }
    const NodeContainer& at(NodeHandle handle) const { return nodes[handle.index]; }

public:
    ~DirectedAcyclicGraph() {
        forEachNode([&](NodeHandle handle) {
            nodes[handle.index].node.destroy();
        });
        delete[] nodes;
    }
    NodeHandle newNode(const Node& node) {
        idx_t index = newIndex();
        nodes[index].node.initialize(node);
        return { index };
    }
    void addChild(NodeHandle parent, NodeHandle child) {
        at(child).parents.insert(parent.index);
    }
    void removeChild(NodeHandle parent, NodeHandle child) {
        at(child).parents.remove(parent.index);
    }
    Node& operator[](NodeHandle handle) { return at(handle).node.value; }
    const Node& operator[](NodeHandle handle) const { return at(handle); }

    ArrayView<const NodeHandle> parents(NodeHandle handle) const { return at(handle).parents; }
    template<typename Callback>
    void forEachNode(Callback&& callback) const {
        indexAllocator.enumerateUsedIdRanges([&](idx_t begin, size_t size) {
            for (idx_t i = 0; i < size; i++)
                callback(NodeHandle { begin + i });
        });
    }

    class Marker {
    private:
        BitSet marks;
        Marker(int_t size) { marks.reserve(size); }
        friend DirectedAcyclicGraph;

    public:
        Marker() = default;

        void clearAll() { marks.clear(); }
        void mark(NodeHandle n) { marks.setBitUnchecked(n.index); }
        void clear(NodeHandle n) { marks.clearBitUnchecked(n.index); }
        void flipMark(NodeHandle n) { marks.flipBitUnchecked(n.index); }
        bool marked(NodeHandle n) const { return marks.getUnchecked(n.index); }
        int_t markCount() const { return marks.popcount(); }
        template<typename Callback>
        void forEachMarked(Callback&& callback) const {
            marks.forEachSetBit([&](size_t idx) {
                callback(NodeHandle { static_cast<idx_t>(idx) });
            });
        }
    };
    Marker newMarker() const { return { nodeCapacity }; }
    void invert(Marker& marker) const {
        forEachNode([&](NodeHandle node) {
            marker.flipMark(node.index);
        });
    }

    class UnsafeTopologicalOrdering {
    public:
        Marker visited;
    };
    class TopologicalOrdering {
    public:
        Marker visited;
        Marker cycleSafeguard;

        int_t cycleLength() { return cycleSafeguard.markCount(); }
        template<typename Callback>
        void forEachNodeInCycle(Callback&& callback) const {
            cycleSafeguard.template forEachMarked<Callback>(std::forward<Callback>(callback));
        }
    };
    TopologicalOrdering newOrdering() const { return { newMarker(), newMarker() }; }
    UnsafeTopologicalOrdering newUnsafeOrdering() const { return { newMarker() }; }
    template<typename Callback>
    void orderParents(UnsafeTopologicalOrdering& order, NodeHandle node, Callback&& callback) const {
        if (order.visited.get(node.index))
            return;
        for (NodeHandle parent : parents(node))
            orderParents<Callback>(order, parent, std::forward<Callback>(callback));
        order.visited.mark(node);
        callback(node);
    }
    template<typename Callback>
    bool orderParents(TopologicalOrdering& order, NodeHandle node, Callback&& callback) const {
        if (order.cycleSafeguard.marked(node))
            return false;
        if (order.visited.marked(node.index))
            return true;
        order.cycleSafeguard.mark(node);
        for (NodeHandle parent : parents(node)) {
            if (!orderParents<Callback>(order, parent, std::forward<Callback>(callback)))
                return false;
        }
        order.cycleSafeguard.clear(node);
        order.visited.mark(node);
        callback(node);
        return true;
    }
    template<typename Callback>
    bool topologicalOrdering(Callback&& callback) const {
        TopologicalOrdering order = newOrdering();
        bool abort = false;
        forEachNode([&](NodeHandle node) {
            if (abort)
                return;
            if (!orderParents<Callback>(order, node, std::forward<Callback>(callback)))
                abort = true;
        });
        return !abort;
    }
    template<typename Callback>
    void unsafeTopologicalOrdering(Callback&& callback) const {
        UnsafeTopologicalOrdering order = newUnsafeOrdering();
        forEachNode([&](NodeHandle node) {
            orderParents<Callback>(order, node, std::forward<Callback>(callback));
        });
    }
};

#pragma once

template<typename GraphHandle>
class BreadthFirstSearch {
private:
    using NodeHandle = GraphHandle::NodeHandle;
    using link_t = GraphHandle::link_t;

    struct QueueItem {
        NodeHandle node;
        NodeHandle parent;
        link_t index;
    };
    std::deque<QueueItem> queue;

    template<bool checkParent>
    void queueChildren(NodeHandle node, NodeHandle parent, [[context]] GraphHandle graph) {
        for (link_t i = 0; i < graph->linkCount(node); i++) {
            NodeHandle child = graph->followLink(node, i);
            if (!checkParent || child != parent)
                queue.push_back({ child, node, i });
        }
    }

public:
    static constexpr size_t FLAG_DONT_EXPLORE = 1;
    static constexpr size_t FLAG_PAUSE_ITERATION = 2;

    template<typename Callback>
    void searchChildren(NodeHandle node, Callback&& callback, [[context]] GraphHandle graph) {
        BreadthFirstSearch(node).iterate(callback);
    }

    BreadthFirstSearch() { }
    BreadthFirstSearch(NodeHandle node, [[context]] GraphHandle graph) {
        searchChildren(node, graph);
    }

    void queueChildren(NodeHandle node, [[context]] GraphHandle graph) {
        queueChildren<false>(node, nullptr);
    }
    void queueChildrenExceptParent(NodeHandle node, NodeHandle parent, [[context]] GraphHandle graph) {
        queueChildren<true>(node, parent);
    }

    void clear() { queue.clear(); }

    template<typename Callback>
    void iterate(Callback&& callback, [[context]] GraphHandle graph) {
        while (!queue.empty()) {
            QueueItem item = queue.front();
            queue.pop_front();
            size_t dec = callback(item.node, item.parent, item.index);
            if (dec & FLAG_DONT_EXPLORE == 0)
                queueChildren<!GraphHandle::IS_ONE_WAY>(item.node, item.parent);
            if (dec & FLAG_PAUSE_ITERATION)
                break;
        }
    }
};

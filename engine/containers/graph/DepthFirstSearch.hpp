#pragma once

template<typename GraphHandle>
class DepthFirstSearch {
private:
    using NodeHandle = GraphHandle::NodeHandle;
    using link_t = GraphHandle::link_t;

    struct PathItem {
        PathItem(NodeHandle node)
            : node(node) { }
        NodeHandle node;
        link_t index = 0;
    };
    std::vector<PathItem> path;

public:
    static constexpr size_t FLAG_DONT_EXPLORE = 1;
    static constexpr size_t FLAG_PAUSE_ITERATION = 2;

    template<typename Callback>
    static void searchChildren(NodeHandle node, Callback&& callback, [[context]] GraphHandle graph) {
        DepthFirstSearch(node).iterate(callback);
    }

    DepthFirstSearch() { }
    DepthFirstSearch(NodeHandle node) {
        reset(node);
    }

    void reset(NodeHandle node) {
        clear();
        path.push_back({ node });
    }

    void clear() { path.clear(); }

    template<typename Callback>
    void iterate(Callback&& callback, [[context]] GraphHandle graph) {
        while (!path.empty()) {
            PathItem parent = path.back();
            NodeHandle current = graph->followLink(parent.node, parent.index);
            size_t dec;
            if (!GraphHandle::IS_ONE_WAY && path.size() > 1 && current == path[path.size() - 2].node)
                dec = FLAG_DONT_EXPLORE;
            else
                dec = callback(current, parent.node, parent.index);

            if ((dec & FLAG_DONT_EXPLORE) || graph->linkCount(current) == 0) {
                do {
                    PathItem& item = path.back();
                    item.index += 1;
                    if (item.index < graph->linkCount(item.node))
                        break;

                    path.pop_back();
                } while (!path.empty());
            } else
                path.push_back({ current });

            if (dec & FLAG_PAUSE_ITERATION)
                break;
        }
    }
};

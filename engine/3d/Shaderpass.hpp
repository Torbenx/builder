#pragma once

#include "math/solver/LinearProgramming.hpp"
#include "util/IdAllocator.hpp"
#include <set>
#include <unordered_map>

class Shaderpass;
class Scene;

struct Renderpass {
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<Shaderpass> passes;
};

struct EffectTriple {
    EffectInstance* effect;
    int_t rank;
    int_t slot;
};

class BindingsTree {
public:
    struct Node {
        bool isLeaf;
        union {
            struct {
                EffectInstance* effect;
                int_t slot;
            } internal;
            struct {
                int_t objectId;
            } leaf;
        } data;
        std::set<Node> children;

        Node(int_t objectId)
            : isLeaf(true) { data.leaf.objectId = objectId; }
        Node(EffectInstance* effect, int_t slot)
            : isLeaf(false) {
            data.internal.effect = effect;
            data.internal.slot = slot;
        }

        bool operator<(const Node& other) const {
            return isLeaf ? (data.leaf.objectId < other.data.leaf.objectId) : (data.internal.effect < other.data.internal.effect);
        }
        bool operator<(uintptr_t effect) const { return ((uintptr_t)data.internal.effect) < effect; }
        bool operator<(int_t objectId) const { return data.leaf.objectId < objectId; }
    };

    std::set<Node> root;
    int_t depth;
    void addEntry(EffectTriple* effects, int_t objectId);

    BindingsTree(int_t depth)
        : depth(depth) { }
};

class Shaderpass {
public:
    struct Entry {
        id_t objectId;
        std::vector<id_t> effectIds;
    };

    id_t shaderId;
    std::vector<Entry> entries;
    std::unordered_map<EffectInstance*, uint32_t> map;
    std::vector<EffectInstance*> inverseMap;
    IdAllocator<uint32_t> idAllocator;
    LinearProgramming<> solver;
    BindingsTree tree;

    Shaderpass(id_t shaderId);

    void draw(Scene&, Renderpass&);
    void addEntry(int_t objectId, std::vector<EffectInstance*>& effects);
    void drawNodes(Scene&, Framebuffer&, EffectShader&, const std::set<BindingsTree::Node>& nodes, int_t level);
};

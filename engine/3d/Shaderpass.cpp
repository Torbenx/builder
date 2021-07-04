#include "3d/Shaderpass.hpp"
#include "3d/Renderpass.hpp"

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
        std::vector<Node> children;

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

    std::vector<Node> root;
    int_t depth;
    void addEntry(EffectTriple* effects, int_t objectId);
    void clear() { root.clear(); }
    void debug(const std::vector<Node>& nodes, int_t level);
    void debug() { debug(root, 0); }

    BindingsTree(int_t depth)
        : depth(depth) { }
};

template<uint32_t N>
class ShaderpassImpl : public Shaderpass {
public:
    struct Entry {
        id_t objectId;
        std::vector<id_t> effectIds;
    };

    std::vector<Entry> entries;
    std::unordered_map<EffectInstance*, uint32_t> map;
    std::vector<EffectInstance*> inverseMap;
    IdAllocator<uint32_t> idAllocator;
    LinearProgramming<N> solver;
    BindingsTree tree;

    ShaderpassImpl(Resource<id_t> shaderHandle);

    void draw(GraphicsObjectCollection&, Renderpass&) override;
    void addEntry(int_t objectId, std::vector<EffectInstance*>& effects) override;
    void drawNodes(GraphicsObjectCollection&, Framebuffer&, EffectShader&, const std::vector<BindingsTree::Node>& nodes, int_t level);
};

std::unique_ptr<Shaderpass> Shaderpass::construct(Resource<id_t> shaderHandle) {
    switch (EffectShaderFactory::slotCount(*shaderHandle)) {
    case 1:
        return std::make_unique<ShaderpassImpl<1>>(shaderHandle);
    case 2:
        return std::make_unique<ShaderpassImpl<2>>(shaderHandle);
    case 3:
        return std::make_unique<ShaderpassImpl<3>>(shaderHandle);
    case 4:
        return std::make_unique<ShaderpassImpl<4>>(shaderHandle);
    case 5:
        return std::make_unique<ShaderpassImpl<5>>(shaderHandle);
    }
}

void BindingsTree::addEntry(EffectTriple* effects, int_t objectId) {
    std::vector<Node>* nodes = &root;
    for (int_t level = 0; level < depth; level++) {
        auto it = std::lower_bound(nodes->begin(), nodes->end(), (uintptr_t)effects[level].effect);
        if (it == nodes->end() || it->data.internal.effect != effects[level].effect) {
            it = nodes->insert(it, Node(effects[level].effect, effects[level].slot));
        } else if (it->data.internal.slot != effects[level].slot) {
            for (int_t i = level + 1; i < depth; i++) {
                if (effects[i].slot == it->data.internal.slot) {
                    effects[i].slot = effects[level].slot;
                    break;
                }
            }
        }
        nodes = &it->children;
    }
    nodes->push_back(objectId);
}

void BindingsTree::debug(const std::vector<Node>& nodes, int_t level) {
    std::string spaces(level * 4, ' ');
    for (const Node& node : nodes) {
        if (level == depth) {
            LOGD("%s%lld", spaces.c_str(), node.data.leaf.objectId);
        } else {
            LOGD("%s%p", spaces.c_str(), node.data.internal.effect);
            debug(node.children, level + 1);
        }
    }
}

template<uint32_t N>
ShaderpassImpl<N>::ShaderpassImpl(Resource<id_t> shaderHandle)
    : Shaderpass(shaderHandle), tree(N) { }

template<uint32_t N>
void ShaderpassImpl<N>::addEntry(int_t objectId, std::vector<EffectInstance*>& effects) {
    Entry& entry = entries.emplace_back();
    entry.objectId = objectId;
    id_t ids[effects.size()];
    for (int_t i = 0; i < effects.size(); i++) {
        EffectInstance* effect = effects[i];
        auto it = map.find(effect);
        if (it == map.end()) {
            ids[i] = idAllocator.allocateId();
            map.insert({ effect, ids[i] });
            if (ids[i] >= inverseMap.size()) {
                inverseMap.resize(ids[i] + 1);
            }
            inverseMap[ids[i]] = effect;
            if (ids[i] >= solver.basisSize) {
                solver.addVariable();
            }
        } else {
            ids[i] = it->second;
        }
        entry.effectIds.push_back(ids[i]);
    }
    solver.addConstraint(ids);
}

template<uint32_t N>
void ShaderpassImpl<N>::draw(GraphicsObjectCollection& objects, Renderpass& renderpass) {
    solver.solve();

    tree.clear();
    EffectTriple triples[N];
    for (Entry& entry : entries) {
        for (int_t i = 0; i < N; i++) {
            triples[i].effect = inverseMap[entry.effectIds[i]];
            triples[i].rank = solver.output[entry.effectIds[i]];
            triples[i].slot = i;
        }
        std::sort(triples, triples + N,
            [](const EffectTriple& a, const EffectTriple& b) {
                return a.rank > b.rank;
            });
        tree.addEntry(triples, entry.objectId);
    }

    EffectShader& shader = EffectShaderFactory::instance(*shaderHandle);
    shader.use();
    shader.uniformMat4x4(shader.viewProjectionUniform, renderpass.viewProjectionMatrix);
    drawNodes(objects, *renderpass.framebuffer, shader, tree.root, 0);
}

template<uint32_t N>
void ShaderpassImpl<N>::drawNodes(GraphicsObjectCollection& objects, Framebuffer& framebuffer, EffectShader& shader, const std::vector<BindingsTree::Node>& nodes, int_t level) {
    for (const BindingsTree::Node& node : nodes) {
        if (level == N) {
            auto& object = objects[node.data.leaf.objectId];
            shader.uniformMat4x4(shader.modelUniform, object.modelMatrix);
            shader.attribPointer(shader.positionAttrib, object.vertices);
            shader.attribPointer(shader.normalAttrib, object.normals);
            framebuffer.draw(object.drawMode, object.indices);
        } else {
            node.data.internal.effect->applyTo(shader.ordered(shader.slots[node.data.internal.slot]));
            drawNodes(objects, framebuffer, shader, node.children, level + 1);
        }
    }
}

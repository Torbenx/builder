#include "3d/Shaderpass.hpp"
#include "3d/Scene.hpp"

void BindingsTree::addEntry(EffectTriple* effects, int_t objectId) {
    std::set<Node>* nodes = &root;
    for (int_t level = 0; level < depth; level++) {
        auto it = nodes->lower_bound((uintptr_t)effects[level].effect);
        if (it == nodes->end() || it->data.internal.effect != effects[level].effect) {
            it = nodes->emplace_hint(it, effects[level].effect, effects[level].slot);
        } else if (it->data.internal.slot != effects[level].slot) {
            for (int_t i = level + 1; i < depth; i++) {
                if (effects[i].slot == it->data.internal.slot) {
                    effects[i].slot = effects[level].slot;
                    break;
                }
            }
        }
        nodes = const_cast<std::set<Node>*>(&it->children);
    }
    nodes->emplace(objectId);
}

void BindingsTree::debug(const std::set<Node>& nodes, int_t level) {
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

Shaderpass::Shaderpass(Resource<id_t> shaderHandle)
    : shaderHandle(shaderHandle), solver(EffectShaderFactory::slotCount(*shaderHandle)), tree(EffectShaderFactory::slotCount(*shaderHandle)) { }

void Shaderpass::addEntry(int_t objectId, std::vector<EffectInstance*>& effects) {
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

void Shaderpass::draw(Scene& scene, Renderpass& renderpass) {
    solver.solve();

    tree.clear();
    EffectTriple triples[solver.n];
    for (Entry& entry : entries) {
        for (int_t i = 0; i < solver.n; i++) {
            triples[i].effect = inverseMap[entry.effectIds[i]];
            triples[i].rank = solver.output[entry.effectIds[i]];
            triples[i].slot = i;
        }
        std::sort(triples, triples + solver.n,
            [](const EffectTriple& a, const EffectTriple& b) {
                return a.rank > b.rank;
            });
        tree.addEntry(triples, entry.objectId);
    }

    EffectShader& shader = EffectShaderFactory::instance(*shaderHandle);
    shader.use();
    shader.uniformMat4x4(shader.viewProjectionUniform, renderpass.viewProjectionMatrix);
    drawNodes(scene, *renderpass.framebuffer, shader, tree.root, 0);
}

void Shaderpass::drawNodes(Scene& scene, Framebuffer& framebuffer, EffectShader& shader, const std::set<BindingsTree::Node>& nodes, int_t level) {
    for (const BindingsTree::Node& node : nodes) {
        if (level == tree.depth) {
            auto& object = scene.objects[node.data.leaf.objectId].object;
            shader.uniformMat4x4(shader.modelUniform, object.modelMatrix);
            shader.attribPointer(shader.positionAttrib, object.vertices);
            shader.attribPointer(shader.normalAttrib, object.normals);
            framebuffer.draw(object.drawMode, object.indices);
        } else {
            node.data.internal.effect->applyTo(shader.ordered(shader.slots[node.data.internal.slot]));
            drawNodes(scene, framebuffer, shader, node.children, level + 1);
        }
    }
}

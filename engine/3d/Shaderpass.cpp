#include "3d/Shaderpass.hpp"
#include "3d/Renderpass.hpp"
#include "containers/BitSet.hpp"

template<uint32_t N>
class ShaderpassImpl : public Shaderpass {
public:
    struct Entry {
        id_t objectId;
        std::array<Effect::Handle, N> effects;
    };
    std::vector<Entry> entries;
    std::array<std::unordered_map<uintptr_t, int_t>, N> counts;
    std::array<int_t, N> indices;

    ShaderpassImpl(Resource<id_t> shaderHandle);

    void draw(GraphicsObjectCollection&, Renderpass&) override;
    void addEntry(id_t objectId, std::vector<Effect::Handle>& effects) override;
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
    return {};
}

template<uint32_t N>
ShaderpassImpl<N>::ShaderpassImpl(Resource<id_t> shaderHandle)
    : Shaderpass(shaderHandle) {
    for (int_t i = 0; i < N; i++) {
        indices[i] = i;
    }
}

template<uint32_t N>
void ShaderpassImpl<N>::addEntry(id_t objectId, std::vector<Effect::Handle>& effects) {
    for (int_t i = 0; i < N; i++) {
        auto& map = counts[i];
        uintptr_t key = (uintptr_t)effects[i].id;
        auto it = map.find(key);
        if (it == map.end()) {
            it = map.insert({ key, 0 }).first;
        }
        it->second++;
    }
    Entry& entry = entries.emplace_back();
    entry.objectId = objectId;
    std::memcpy(entry.effects.data(), effects.data(), N * sizeof(Effect::Handle));

    std::stable_sort(indices.begin(), indices.end(), [&](const int_t& a, const int_t& b) {
        return counts[a].size() < counts[b].size();
    });

    std::sort(entries.begin(), entries.end(), [&](const Entry& a, const Entry& b) {
        for (int_t i = 0; i < N; i++) {
            Effect::Handle ae = a.effects[indices[i]];
            Effect::Handle be = b.effects[indices[i]];
            if (ae.id == be.id)
                continue;
            return ae.id < be.id;
        }
        return false;
    });

    std::array<BitSet, N> binds;
    std::array<Effect::Handle, N> prev {};
    for (int_t k = 0; k < (int_t)entries.size(); k++) {
        int_t i = 0;
        for (; i < N; i++) {
            int_t j = indices[i];
            if (prev[j].id != entry.effects[j].id) {
                binds[i].setBit(k);
                break;
            }
        }
        for (; i < N; i++) {
            int_t j = indices[i];
            prev[j] = entries[k].effects[j];
        }
    }
}

template<uint32_t N>
void ShaderpassImpl<N>::draw(GraphicsObjectCollection& objects, Renderpass& renderpass) {
    EffectShader& shader = EffectShaderFactory::instance(*shaderHandle);
    shader.use();
    shader.uniformMat4x4(shader.viewProjectionUniform, renderpass.viewProjectionMatrix);
    std::array<sid_t, N> prev;
    prev.fill(std::numeric_limits<sid_t>::max());
    for (Entry& entry : entries) {
        int_t i = 0;
        for (; i < N; i++) {
            int_t j = indices[i];
            if (prev[j] != entry.effects[j].id)
                break;
        }
        for (; i < N; i++) {
            int_t j = indices[i];
            entry.effects[j]->applyTo(shader.ordered(shader.slots[j]));
            prev[j] = entry.effects[j].id;
        }
        GraphicsObject& object = objects[entry.objectId];
        shader.uniformMat4x4(shader.modelUniform, object.modelMatrix);
        shader.attribPointer(shader.positionAttrib, object.vertices);
        shader.attribPointer(shader.normalAttrib, object.normals);
        renderpass.framebuffer->draw(object.drawMode, object.indices);
    }
}

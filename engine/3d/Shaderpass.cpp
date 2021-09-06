#include "3d/Shaderpass.hpp"
#include "3d/Renderpass.hpp"
#include "containers/BitSet.hpp"
#include "containers/DataStream.hpp"

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
    std::array<DataStream, N> levels;

    ShaderpassImpl(Resource<id_t> shaderHandle);

    void draw(GraphicsObjectCollection&, Renderpass&, GAllocator&) override;
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
    {
        Entry& entry = entries.emplace_back();
        entry.objectId = objectId;
        std::memcpy(entry.effects.data(), effects.data(), N * sizeof(Effect::Handle));
    }

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

    for (auto& stream : levels) {
        stream.clear();
    }
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
            prev[j] = entry.effects[j].id;

            auto& stream = levels[j];
            Effect::TypeData data = Effect::data(entry.effects[j].type);
            stream.writeN(data.vertexBuffers.view(entry.effects[j].ptr));
            stream.writeN(data.textures.view(entry.effects[j].ptr));
            if (data.vertexUniformDesc.size > 0)
                stream.write(data.vertexUniformPtr.get(entry.effects[j].ptr));
            if (data.fragmentUniformDesc.size > 0)
                stream.write(data.fragmentUniformPtr.get(entry.effects[j].ptr));
        }
    }
}

template<size_t N, size_t... indices>
std::array<DataStream::Reader, N> createReaderArray_helper(const std::array<DataStream, N>& streams, std::index_sequence<indices...>) {
    return { streams[indices].beginRead()... };
}

template<uint32_t N>
void ShaderpassImpl<N>::draw(GraphicsObjectCollection& objects, Renderpass& renderpass, GAllocator& allocator) {
    EffectShader& shader = EffectShaderFactory::instance(*shaderHandle);
    shader.use();
    shader.uniformMat4x4(shader.viewProjectionUniform, renderpass.viewProjectionMatrix);
    std::array<sid_t, N> prev;
    prev.fill(std::numeric_limits<sid_t>::max());
    auto readers = createReaderArray_helper(levels, std::make_index_sequence<N>());
    for (Entry& entry : entries) {
        int_t i = 0;
        for (; i < N; i++) {
            int_t j = indices[i];
            if (prev[j] != entry.effects[j].id)
                break;
        }
        for (; i < N; i++) {
            int_t j = indices[i];
            prev[j] = entry.effects[j].id;

            DataStream::Reader& reader = readers[j];
            Effect::TypeData data = Effect::data(entry.effects[j].type);
            auto state = shader.ordered(shader.slots[j]);
            for (int_t c = 0; c < data.vertexBuffers.size(); c++)
                state.attribPointer({ allocator.buffer(reader.read<GPtr>()), data.vertexBufferElementSizes[c] });
            for (int_t c = 0; c < data.textures.size(); c++)
                state.uniformTexture(reader.read<Texture>());
            if (data.vertexUniformDesc.size > 0)
                state.uniformBuffer({ allocator.buffer(reader.read<GPtr>()), data.vertexUniformDesc.size });
            if (data.fragmentUniformDesc.size > 0)
                state.uniformBuffer({ allocator.buffer(reader.read<GPtr>()), data.fragmentUniformDesc.size });

            entry.effects[j]->applyTo(state);
        }
        GraphicsObject& object = objects[entry.objectId];
        shader.uniformMat4x4(shader.modelUniform, object.modelMatrix);
        shader.attribPointer(shader.positionAttrib, object.vertices);
        shader.attribPointer(shader.normalAttrib, object.normals);
        renderpass.framebuffer->draw(object.drawMode, object.indices);
    }
}

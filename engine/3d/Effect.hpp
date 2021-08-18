#pragma once

#include "3d/Attribute.hpp"
#include "GL/Shader.hpp"
#include "containers/IdAllocator.hpp"
#include "resource/Resource.hpp"
#include "util/Shaders.hpp"
#include <memory>
#include <unordered_map>

template<typename T>
class EffectImpl;

class Effect {
public:
    static constexpr int_t MAX_TYPES = 128;

    struct Type {
        sid_t id;
    };

    struct TypeData {
        int_t vertexBufferOffset; // offset in bytes form Effect* to start of an array of VertexBuffer
        int_t vertexBufferCount;
        int_t textureOffset; // offset in bytes form Effect* to start of an array of Texture
        int_t textureCount;
    };

    struct Handle {
        Effect::Type type;
        sid_t id;
        Effect* ptr;
        Effect* operator->() { return ptr; }
    };

    virtual void applyTo(OrderedShader) = 0;
    virtual ~Effect() {
        // deallocated type id!
    }

    Effect(Type type)
        : m_handle { type, allocateEffectId(type), this } { }

    Handle handle() { return m_handle; }

private:
    using AddToFunc = void (*)(EffectShaderBuilder&);
    static AddToFunc functions[MAX_TYPES];
    static TypeData typeData[MAX_TYPES];

    static Type registerEffectType(AddToFunc, TypeData);
    static sid_t allocateEffectId(Type);

    Handle m_handle;

    friend class EffectShaderFactory;
    template<typename T>
    friend class EffectImpl;
};

namespace effect_impl {
template<typename T>
concept HasVertexBuffers = requires(T t) { std::is_same_v<decltype(t.vertexBuffers), VertexBuffer[sizeof(t.vertexBuffers) / sizeof(VertexBuffer)]>; };
}

template<typename T>
class EffectImpl : public Effect {
public:
    EffectImpl()
        : Effect(effectType) { }

private:
    static int_t vertexBufferCount() { return 0; }
    static int_t vertexBufferOffset() { return 0; }
    static int_t textureCount() { return 0; }
    static int_t textureOffset() { return 0; }

    static int_t vertexBufferCount() requires effect_impl::HasVertexBuffers<T> { return 1; }
    static int_t vertexBufferOffset() requires effect_impl::HasVertexBuffers<T> { return 1; }

    static Type registerEffectTypeImpl() {
        return registerEffectType(&T::addTo,
            { .vertexBufferOffset = vertexBufferOffset(),
                .vertexBufferCount = vertexBufferCount(),
                .textureOffset = textureOffset(),
                .textureCount = textureCount() });
    }

public:
    static inline const Type effectType = registerEffectTypeImpl();
};

class EffectShaderBuilder : public ShaderBuilder {
public:
    const AttributeList attributes;
    Shader::LocationHandle positionAttrib, normalAttrib, modelUniform, viewProjectionUniform;
    std::vector<OrderedShaderState> slots;

    EffectShaderBuilder(AttributeList attributes);

    void beginEffect() {
        newContext();
        slots.push_back(orderedState());
    }

private:
    std::string buildVertexSource() override;
    std::string buildFragmentSource() override;
    using ShaderBuilder::newContext;
};

class EffectShader : public Shader {
public:
    static constexpr char WORLD_POSITION[] = "v_worldPos";
    static constexpr char WORLD_NORMAL[] = "v_normal";
    static constexpr char SCREEN_POSITION[] = "gl_Position";
    static constexpr char LIGHT_INTENSITY[] = "intensity";
    static constexpr char RAW_POSITION[] = "a_position";
    static constexpr char RAW_NORMAL[] = "a_normal";
    static constexpr char RAW_TO_WORLD_MATRIX[] = "u_model";
    static constexpr char WORLD_TO_SCREEN_MATRIX[] = "u_viewProjection";
    static constexpr char FRAGMENT_DIFFUSE_COLOR[] = "diffuseColor";

    Shader::LocationHandle positionAttrib, normalAttrib, modelUniform, viewProjectionUniform;
    std::vector<OrderedShaderState> slots;

    EffectShader() { }
    EffectShader(EffectShaderBuilder& builder) { initialize(builder); }

    void initialize(EffectShaderBuilder& builder) {
        Shader::initialize(builder);
        positionAttrib = builder.positionAttrib;
        normalAttrib = builder.normalAttrib;
        modelUniform = builder.modelUniform;
        viewProjectionUniform = builder.viewProjectionUniform;
        slots = builder.slots;
    }

private:
    using Shader::initialize;
};

namespace effect_impl {
struct ShaderMonitor;
}

class EffectShaderFactory {
public:
    static Resource<id_t> getHandle(const std::vector<Effect::Handle>&);
    static EffectShader& instance(id_t id) { return shaders[id]; }
    static int_t slotCount(id_t id) { return instance(id).slots.size(); }

private:
    static std::vector<EffectShader> shaders;
    friend struct effect_impl::ShaderMonitor;
};

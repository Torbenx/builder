#pragma once

#include "3d/Attribute.hpp"
#include "GL/Shader.hpp"
#include "GL/std140.hpp"
#include "containers/ArrayView.hpp"
#include "containers/IdAllocator.hpp"
#include "resource/Resource.hpp"
#include "util/Shaders.hpp"
#include <memory>
#include <unordered_map>

template<typename T>
class EffectImpl;

class Effect;

struct VertexBufferDescription {
    enum Format {
        Scalar,
        Vec2,
        Vec3,
        Vec4,
    };
    const char* name;
    Format format;
};

struct TextureDescription {
    enum SamplerType {
        Sampler2D,
        Sampler2DShadow,
        SamplerCube,
        SamplerCubeShadow,
    };
    const char* name;
    SamplerType type;
};

struct UniformBufferDescription {
    int_t size = 0;
};

struct UniformBufferDestination {
    uint16_t bufferId;
    uint16_t offset; // in 16 bytes
};

class Effect {
public:
    static constexpr int_t MAX_TYPES = 128;
    using AddToFunc = void (*)(EffectShaderBuilder&);

    struct Type {
        sid_t id;
    };

    struct TypeData {
        InstructureArray<Effect, const VertexBuffer> vertexBuffers;
        ArrayView<const VertexBufferDescription> vertexBufferDescriptions;

        InstructureArray<Effect, const Texture> textures;
        ArrayView<const TextureDescription> textureDescriptions;

        UniformBufferDescription fragmentUniform;
        InstructureVariable<Effect, UniformBufferDestination> fragmentUniformDestination;

        UniformBufferDescription vertexUniform;
        InstructureVariable<Effect, UniformBufferDestination> vertexUniformDestination;

        AddToFunc addToFunction;
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
    static TypeData typeData[MAX_TYPES];

    static Type registerEffectType(TypeData);
    static sid_t allocateEffectId(Type);

    Handle m_handle;

    friend class EffectShaderFactory;
    template<typename T>
    friend class EffectImpl;
};

namespace effect_impl {

template<typename T, typename ArrayType>
struct ArraySize;

template<typename T, int_t N>
struct ArraySize<T, T[N]> {
    static constexpr int_t n = N;
};

template<typename T>
concept HasVertexBuffers = ArraySize<const VertexBuffer, decltype(T::vertexBuffers)>::n == ArraySize<const VertexBufferDescription, decltype(T::vertexBufferDescriptions)>::n;

template<typename T>
concept HasTextures = ArraySize<const Texture, decltype(T::textures)>::n == ArraySize<const TextureDescription, decltype(T::textureDescriptions)>::n;

template<typename T>
concept HasFragmentUniform = std140::Struct<typename T::FragmentUniformBuffer> && std::is_same_v<decltype(T::fragmentUniformDestination), UniformBufferDestination>;

template<typename T>
concept HasVertexUniform = std140::Struct<typename T::VertexUniformBuffer> && std::is_same_v<decltype(T::vertexUniformDestination), UniformBufferDestination>;
}

template<typename T>
class EffectImpl : public Effect {
public:
    static const Type effectType;

    EffectImpl()
        : Effect(effectType) { }

    static Type registerEffectType() {
        Effect::TypeData data;
        if constexpr (effect_impl::HasVertexBuffers<T>) {
            data.vertexBuffers = &T::vertexBuffers;
            data.vertexBufferDescriptions = T::vertexBufferDescriptions;
        }
        if constexpr (effect_impl::HasTextures<T>) {
            data.textures = &T::textures;
            data.textureDescriptions = T::textureDescriptions;
        }
        if constexpr (effect_impl::HasFragmentUniform<T>) {
            data.fragmentUniform = { sizeof(typename T::FragmentUniformBuffer) };
            data.fragmentUniformDestination = &T::fragmentUniformDestination;
        }
        if constexpr (effect_impl::HasVertexUniform<T>) {
            data.vertexUniform = { sizeof(typename T::VertexUniformBuffer) };
            data.vertexUniformDestination = &T::vertexUniformDestination;
        }
        data.addToFunction = &T::addTo;
        return Effect::registerEffectType(data);
    }
};

class EffectShaderBuilder : public ShaderBuilder {
public:
    const AttributeList attributes;
    Shader::LocationHandle positionAttrib, normalAttrib, modelUniform, viewProjectionUniform;

    EffectShaderBuilder(AttributeList attributes);

private:
    std::string buildVertexSource() override;
    std::string buildFragmentSource() override;
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

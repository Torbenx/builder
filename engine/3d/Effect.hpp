#pragma once

#include "3d/Attribute.hpp"
#include "GL/Shader.hpp"
#include "GL/std140.hpp"
#include "containers/ArrayView.hpp"
#include "containers/IdAllocator.hpp"
#include "containers/InstructureVariable.hpp"
#include "graphics/helpers.hpp"
#include "resource/Resource.hpp"
#include "util/Shaders.hpp"
#include <memory>
#include <unordered_map>

template<typename T>
class EffectImpl;

struct UniformBufferDescription {
    int_t size = 0;
};

class Effect {
public:
    using AddToFunc = void (*)(EffectShaderBuilder&);

    struct TypeData {
        ArrayView<const int_t> vertexBufferElementSizes;
        InstructureArray<Effect, GPtr> vertexBuffers;

        InstructureArray<Effect, Texture> textures;

        UniformBufferDescription fragmentUniformDesc;
        InstructureVariable<Effect, GPtr> fragmentUniformPtr;

        UniformBufferDescription vertexUniformDesc;
        InstructureVariable<Effect, GPtr> vertexUniformPtr;

        AddToFunc addToFunction;
    };

    struct Type {
        Type() = default;
        Type(const TypeData& in)
            : data(&in) { }
        const TypeData* data = nullptr;
    };

    static const TypeData& data(Type type) { return *type.data; }

    struct Handle {
        Effect::Type type;
        Effect* ptr;
        Effect* operator->() { return ptr; }
    };

    ~Effect() {
        // deallocate effect id!
    }

    Effect(Type type)
        : m_handle { type, this } { }

    Handle handle() { return m_handle; }

private:
    static sid_t allocateEffectId(Type);

    Handle m_handle;

    friend class EffectShaderFactory;
    template<typename T>
    friend class EffectImpl;
    friend void init_effect();
};

namespace effect_detail {

template<typename T, typename ArrayType>
struct ArraySize;

template<typename T, int_t N>
struct ArraySize<T, T[N]> {
    static constexpr int_t n = N;
};

template<typename T>
concept HasVertexBuffers = ArraySize<GPtr, decltype(T::vertexBuffers)>::n == ArraySize<const int_t, decltype(T::vertexBufferElementSizes)>::n;

template<typename T>
concept HasTextures = ArraySize<Texture, decltype(T::textures)>::n >= 0;

template<typename T>
concept HasFragmentUniform = std140::Struct<typename T::FragmentUniform> && std::is_same_v<decltype(T::fragmentUniform), UniformHelper<typename T::FragmentUniform>>;

template<typename T>
concept HasVertexUniform = std140::Struct<typename T::VertexUniform> && std::is_same_v<decltype(T::vertexUniform), UniformHelper<typename T::VertexUniform>>;
}

DECLARE_SECTION(Effect::TypeData, effect)

template<typename T>
class EffectImpl : public Effect {
public:
    static TypeData registerEffectType() {
        Effect::TypeData data;
        if constexpr (effect_detail::HasVertexBuffers<T>) {
            data.vertexBufferElementSizes = T::vertexBufferElementSizes;
            data.vertexBuffers = &T::vertexBuffers;
        }
        if constexpr (effect_detail::HasTextures<T>) {
            data.textures = &T::textures;
        }
        if constexpr (effect_detail::HasFragmentUniform<T>) {
            data.fragmentUniformDesc = { sizeof(typename T::FragmentUniform) };
            data.fragmentUniformPtr = InstructureVariable { InstructureVariable { &T::fragmentUniform }, InstructureVariable { &UniformHelper<typename T::FragmentUniform>::ptr } };
        }
        if constexpr (effect_detail::HasVertexUniform<T>) {
            data.vertexUniformDesc = { sizeof(typename T::VertexUniform) };
            data.vertexUniformPtr = InstructureVariable { InstructureVariable { &T::vertexUniform }, InstructureVariable { &UniformHelper<typename T::VertexUniform>::ptr } };
        }
        data.addToFunction = &T::addTo;
        return data;
    }

    static inline IN_SECTION(effect) TypeData effectType = registerEffectType();

    EffectImpl()
        : Effect(effectType) { }
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

namespace effect_detail {
struct ShaderMonitor;
}

class EffectShaderFactory {
public:
    static Resource<id_t> getHandle(const std::vector<Effect::Handle>&);
    static EffectShader& instance(id_t id) { return shaders[id]; }
    static int_t slotCount(id_t id) { return instance(id).slots.size(); }

private:
    static std::vector<EffectShader> shaders;
    friend struct effect_detail::ShaderMonitor;
};

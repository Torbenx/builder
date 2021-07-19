#pragma once

#include "3d/Attribute.hpp"
#include "GL/Shader.hpp"
#include "containers/IdAllocator.hpp"
#include "resource/Resource.hpp"
#include "util/Shaders.hpp"
#include <memory>
#include <unordered_map>

class Effect {
public:
    using AddToFunc = void (*)(EffectShaderBuilder&);
    static constexpr int_t maxTypes = 128;

    struct Type {
        sid_t id;
    };

    struct Handle {
        Effect::Type type;
        sid_t id;
        Effect *ptr;
        Effect* operator->() { return ptr; }
    };

    template<typename T>
    static Type registerEffect() {
        return registerEffect_internal(&T::addTo, sizeof(T));
    }

    Effect(Type);

    Handle handle() { return m_handle; }

    virtual void applyTo(OrderedShader) = 0;
    virtual ~Effect() { }

private:
    static inline AddToFunc functions[maxTypes];

    static Type registerEffect_internal(AddToFunc, int_t size);
    friend class EffectShaderFactory;

    Handle m_handle;
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

#include "3d/Effect.hpp"
#include <algorithm>

std::vector<EffectShader> EffectShaderFactory::shaders;

Effect::AddToFunc Effect::functions[Effect::MAX_TYPES];
Effect::TypeData Effect::typeData[Effect::MAX_TYPES];

namespace effect_impl {

IdAllocator<> shaderIdAllocator;
std::unordered_map<uintptr_t, std::unique_ptr<ShaderMonitor>> shaderHandles;

struct ShaderMonitor final : public ResourceMonitor {
    id_t id;
    uintptr_t hash;

    ShaderMonitor(id_t id, uintptr_t hash)
        : id(id), hash(hash) { }

    bool release() override {
        EffectShaderFactory::shaders[id].release();
        shaderIdAllocator.freeId(id);
        shaderHandles.erase(hash);
        return false;
    }
};

std::vector<sIdAllocator> effectIdAllocators;

}

sid_t Effect::allocateEffectId(Effect::Type type) {
    VDUMP(type.id);
    if (effect_impl::effectIdAllocators.size() <= type.id) {
        effect_impl::effectIdAllocators.resize(type.id + 1);
    }
    return effect_impl::effectIdAllocators[type.id].allocateId();
}

Effect::Type Effect::registerEffectType(AddToFunc addTo, TypeData data) {
    static sid_t nextId = 0;
    VDUMP(nextId);
    EXPECT(Math, data.vertexBufferCount, (int_t)0);
    functions[nextId] = addTo;
    typeData[nextId] = data;
    return { nextId++ };
}

EffectShaderBuilder::EffectShaderBuilder(AttributeList attributes)
    : ShaderBuilder()
    , attributes(attributes) {
    addVarying("vec3", EffectShader::WORLD_POSITION);
    addVarying("vec3", EffectShader::WORLD_NORMAL);
    positionAttrib = addAttrib("vec3", EffectShader::RAW_POSITION);
    normalAttrib = addAttrib("vec3", EffectShader::RAW_NORMAL);
    modelUniform = addVertexUniform("mat4", EffectShader::RAW_TO_WORLD_MATRIX);
    viewProjectionUniform = addVertexUniform("mat4", EffectShader::WORLD_TO_SCREEN_MATRIX);

    vMain << EffectShader::WORLD_POSITION << " = (" << EffectShader::RAW_TO_WORLD_MATRIX << " * vec4(" << EffectShader::RAW_POSITION << ", 1.0) ).xyz;\n";
    vMain << EffectShader::WORLD_NORMAL << " = (" << EffectShader::RAW_TO_WORLD_MATRIX << " * vec4(" << EffectShader::RAW_NORMAL << ", 0.0) ).xyz;\n";
    vMain << EffectShader::WORLD_NORMAL << " = normalize(" << EffectShader::WORLD_NORMAL << ");\n";
    vMain << "gl_Position = " << EffectShader::WORLD_TO_SCREEN_MATRIX << " * vec4(" << EffectShader::WORLD_POSITION << ", 1.0);\n";
}

std::string EffectShaderBuilder::buildVertexSource() {
    std::ostringstream v;
    v << vExt.str() << vInit.str() << "void main(){\n"
      << vMain.str() << "}";
    return v.str();
}

std::string EffectShaderBuilder::buildFragmentSource() {
    std::ostringstream f;

    f << fExt.str() << fInit.str() << "out vec4 " << Shader::FRAGMENT_COLOR << ";\n"
      << "void main(){\n";
    f << "vec3 " << EffectShader::LIGHT_INTENSITY << " = vec3(0.0);\n";
    f << "vec3 " << EffectShader::FRAGMENT_DIFFUSE_COLOR << " = vec3(0.0);\n";
    f << Shader::FRAGMENT_COLOR << " = vec4(0.0, 0.0, 0.0, 1.0);\n";
    f << fMain.str();
    f << Shader::FRAGMENT_COLOR << " += vec4(" << EffectShader::FRAGMENT_DIFFUSE_COLOR << " * " << EffectShader::LIGHT_INTENSITY << ", 0.0);\n}";

    return f.str();
}

Resource<id_t> EffectShaderFactory::getHandle(const std::vector<Effect::Handle>& effects) {
    uintptr_t hash = 0;
    for (Effect::Handle effect : effects) {
        hash ^= (uintptr_t)effect.type.id + 1;
    }

    std::unique_ptr<effect_impl::ShaderMonitor>& ptr = effect_impl::shaderHandles[hash];
    if (ptr) {
        return Resource<id_t>(ptr->id, &*ptr);
    }
    id_t id = effect_impl::shaderIdAllocator.allocateId();
    ptr.reset(new effect_impl::ShaderMonitor(id, hash));
    if (id == shaders.size()) {
        shaders.resize(id + 1);
    }
    AttributeList attributes;
    EffectShaderBuilder builder(attributes);
    for (Effect::Handle effect : effects) {
        Effect::functions[effect.type.id](builder);
    }
    shaders[id].initialize(builder);
    return Resource<id_t>(id, &*ptr);
}

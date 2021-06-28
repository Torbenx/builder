#include "3d/Effect.hpp"

const std::string EffectShader::WORLD_POSITION = "v_worldPos";
const std::string EffectShader::WORLD_NORMAL = "v_normal";
const std::string EffectShader::SCREEN_POSITION = "gl_Position";
const std::string EffectShader::LIGHT_INTENSITY = "intensity";
const std::string EffectShader::RAW_POSITION = "a_position";
const std::string EffectShader::RAW_NORMAL = "a_normal";
const std::string EffectShader::RAW_TO_WORLD_MATRIX = "u_model";
const std::string EffectShader::WORLD_TO_SCREEN_MATRIX = "u_viewProjection";
const std::string EffectShader::FRAGMENT_DIFFUSE_COLOR = "diffuseColor";

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

Resource<EffectShaderHandle> EffectShaderFactory::getHandle(const std::vector<EffectInstance*>& effects) {
    uintptr_t hash = 0;
    for (EffectInstance* instance : effects) {
        hash ^= ((uintptr_t)instance->effect) >> 3;
    }

    /*LOGD("hash: %llx", hash);
    for (EffectInstance* instance : effects) {
        LOGD("0x%p: 0x%p", instance, instance->effect);
    }
    LOGD("");*/

    Resource<EffectShaderHandle>::Weak& w = shaderHandles[hash];
    if (w.expired()) {
        id_t id = idAllocator.allocateId();
        if (id == shaders.size()) {
            shaders.resize(id + 1);
        }
        AttributeList attributes;
        EffectShaderBuilder builder(attributes);
        for (EffectInstance* effect : effects) {
            effect->effect->addTo(builder);
        }
        shaders[id].initialize(builder);
        Resource<EffectShaderHandle> ret({ id });
        w = ret.weak();
        return ret;
    } else {
        return w.acquire();
    }
}

void EffectShaderHandle::release() {
    EffectShaderFactory::shaders[id].release();
}

#include "lgraphic/LShader.hpp"
#include "util/log.hpp"

const std::string LShader::WORLD_POSITION = "v_worldPos";
const std::string LShader::WORLD_NORMAL = "v_normal";
const std::string LShader::SCREEN_POSITION = "gl_Position";
const std::string LShader::LIGHT_INTENSITY = "intensity";

LShaderBuilder::LShaderBuilder()
    : ShaderBuilder() {
    addVarying("vec3", LShader::WORLD_POSITION);
    addVarying("vec3", LShader::WORLD_NORMAL);
}

LShader::EffectLocationHandle LShaderBuilder::addEffect(const LShader::EffectData& data) {
    int handle = this->data.size();
    this->data.push_back(data);
    return { handle };
}

std::string LShaderBuilder::buildVertexSource() {
    std::ostringstream v;
    v << vExt.str() << vInit.str() << "void main(){\n";

    v << LShader::WORLD_POSITION << " = (";
    if (!rawToWorldMatrix.empty()) {
        v << rawToWorldMatrix << " * ";
    }
    v << "vec4(" << rawPosition << ", 1.0) ).xyz;\n";

    v << LShader::WORLD_NORMAL << " = (";
    if (!rawToWorldMatrix.empty()) {
        v << rawToWorldMatrix << " * ";
    }
    v << "vec4(" << rawNormal << ", 0.0) ).xyz;\n";
    v << LShader::WORLD_NORMAL << " = normalize(" << LShader::WORLD_NORMAL << ");\n";

    v << "gl_Position = ";
    if (!worldToScreenMatrix.empty()) {
        v << worldToScreenMatrix << " * ";
    }
    v << "vec4(" << LShader::WORLD_POSITION << ", 1.0);\n";

    v << vMain.str() << "}";

    return v.str();
}

std::string LShaderBuilder::buildFragmentSource() {
    std::ostringstream f;

    f << fExt.str() << fInit.str() << "out vec4 " << Shader::FRAGMENT_COLOR << ";\n"
      << "void main(){\n";
    f << "vec3 " << LShader::LIGHT_INTENSITY << " = vec3(0.0);\n";

    f << fMain.str() << Shader::FRAGMENT_COLOR << " = vec4(";

    f << fragmentColor;
    if (!fragmentColor.empty())
        f << " + ";

    f << fragmentDiffuseColor;
    if (!fragmentDiffuseColor.empty())
        f << " * " << LShader::LIGHT_INTENSITY;

    if (fragmentColor.empty() && fragmentDiffuseColor.empty())
        f << "vec3(0.0)";

    f << ", 1.0);\n}";

    return f.str();
}

LShader::LShader(LShaderBuilder& builder)
    : Shader(builder), data(builder.data) { }

void LShader::bindEffect(EffectLocationHandle handle, Effect& effect) {
    effect.bindTo(*this, data[handle.handle]);
}

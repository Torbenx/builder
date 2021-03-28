#include "LShader.hpp"
#include "../util/log.hpp"

const std::string LShader::WORLD_POSITION = "v_worldPos";
const std::string LShader::WORLD_NORMAL = "v_normal";
const std::string LShader::SCREEN_POSITION = "gl_Position";
const std::string LShader::LIGHT_INTENSITY = "intensity";

LShaderBuilder::LShaderBuilder() : ShaderBuilder(){
    addVarying("vec3", LShader::WORLD_POSITION);
    addVarying("vec3", LShader::WORLD_NORMAL);
}

void LShaderBuilder::addEffect(LShader::Effect& effect, const LShader::EffectData& data){
    LShader::EffectAndData effectAndData;
    effectAndData.effect = &effect;
    effectAndData.data = data;
    this->data.push_back(effectAndData);
}

std::string LShaderBuilder::buildVertexSource(){
    std::ostringstream v;
    v << vInit.str() << "void main(){\n";

    v << LShader::WORLD_POSITION << " = (";
    if(!rawToWorldMatrix.empty()){
        v << rawToWorldMatrix << " * ";
    }
    v << "vec4(" << rawPosition << ", 1.0) ).xyz;\n";

    v << LShader::WORLD_NORMAL << " = (";
    if(!rawToWorldMatrix.empty()){
        v << rawToWorldMatrix << " * ";
    }
    v << "vec4(" << rawNormal << ", 0.0) ).xyz;\n";
    v << LShader::WORLD_NORMAL << " = normalize(" << LShader::WORLD_NORMAL << ");\n";

    v << "gl_Position = ";
    if(!worldToScreenMatrix.empty()){
        v << worldToScreenMatrix << " * ";
    }
    v << "vec4(" << LShader::WORLD_POSITION << ", 1.0);\n";

    v << vMain.str() << "}";

    return v.str();
}

std::string LShaderBuilder::buildFragmentSource(){
    std::ostringstream f;

    f << fInit.str() << "void main(){\n";
    f << "vec3 " << LShader::LIGHT_INTENSITY << " = vec3(0.0);\n";

    f << fMain.str() << "gl_FragColor = vec4(";

    f << fragmentColor;
    if(!fragmentColor.empty())
        f << " + ";

    f << fragmentDiffuseColor;
    if(!fragmentDiffuseColor.empty())
        f << " * " << LShader::LIGHT_INTENSITY;

    if(fragmentColor.empty() && fragmentDiffuseColor.empty())
        f << "vec3(0.0)";

    f<<", 1.0);\n}";

    return f.str();
}

LShader::LShader(LShaderBuilder& builder) : Shader(builder), data(builder.data) {

}

void LShader::applyEffects(){
    for(LShader::EffectAndData& data : this->data){
        data.effect->apply(*this, data.data);
    }
}

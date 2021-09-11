#include "3d/SpotLight.hpp"
#include "util/log.hpp"

template<>
const Effect::Type EffectImpl<SpotLight>::effectType = registerEffectType();
SpotLight::SpotLight() { }

void SpotLight::addTo(EffectShaderBuilder& s) {
    std::string mvpUniform = s.uniqueName("u_mvpMatrix");
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string positionVarying = s.uniqueName("v_position");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    s.addFragmentUniform("sampler2DShadow", textureUniform);
    s.nextTextureSlot();
    s.beginVertexUniformBuffer("SpotLightVert");
    s.vInit << "mat4 " << mvpUniform << ";\n";
    s.endVertexUniformBuffer();
    s.beginFragmentUniformBuffer("SpotLightFrag");
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.fInit << "vec3 " << lightPosUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.addVarying("vec3", positionVarying);
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("direction");
    std::string pos = s.uniqueName("pos");
    s.vMain << "vec4 " << pos << " = " << mvpUniform << " * vec4(" << EffectShader::WORLD_POSITION << ",1.0);\n";
    s.vMain << positionVarying << " = " << pos << ".xyz / " << pos << ".w;\n";
    s.fMain << "vec3 " << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << "," << positionVarying << ");\n";
    s.fMain << "if("
            << "dot(" << positionVarying << ".xy - 0.5," << positionVarying << ".xy - 0.5) <= 0.25 && " << positionVarying << ".z >= 0.0){\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * max(0.0, -dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n}\n";
}

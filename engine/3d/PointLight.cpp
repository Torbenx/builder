#include "3d/PointLight.hpp"
#include "util/log.hpp"
#include <cmath>

template<>
const Effect::Type EffectImpl<PointLight>::effectType = registerEffectType();
PointLight::PointLight() { }

void PointLight::addTo(EffectShaderBuilder& s) {
    std::string textureUniform = s.uniqueName("u_depthMap");
    std::string colorUniform = s.uniqueName("u_color");
    std::string lightPosUniform = s.uniqueName("u_lightPos");
    std::string a = s.uniqueName("u_a");
    std::string b = s.uniqueName("u_b");
    s.addFragmentUniform("samplerCubeShadow", textureUniform);
    s.nextTextureSlot();
    /*s.addFragmentUniform("vec3", colorUniform);
    s.addFragmentUniform("vec3", lightPosUniform);
    s.addFragmentUniform("vec2", factors);*/
    s.beginFragmentUniformBuffer(s.uniqueName("PointLight"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.fInit << "vec3 " << lightPosUniform << ";\n";
    //s.fInit << "float " << s.uniqueName("padding") << ";\n";
    s.fInit << "float " << a << ";\n";
    s.fInit << "float " << b << ";\n";
    s.endFragmentUniformBuffer();
    std::string depth = s.uniqueName("depth");
    std::string result = s.uniqueName("result");
    std::string direction = s.uniqueName("direction");
    s.fMain << "vec3 " << direction << " = " << EffectShader::WORLD_POSITION << " - " << lightPosUniform << ";\n";
    s.fMain << "float " << depth << " = " << a << " - " << b << " / max(abs(" << direction << ".x), max(abs(" << direction << ".y), abs(" << direction << ".z)));\n";
    s.fMain << "float " << result << " = texture(" << textureUniform << ",vec4(" << direction << ", (" << depth << "+1.0) * 0.5 - 0.001));\n";
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << result << " * abs(dot(" << EffectShader::WORLD_NORMAL << ", normalize(" << direction << "))) * " << colorUniform << " / dot(" << direction << "," << direction << ");\n";
}

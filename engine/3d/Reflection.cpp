#include "3d/Reflection.hpp"

template<>
Effect::TypeData IN_SECTION(effect) EffectImpl<Reflection>::effectType = registerEffectType();
Reflection::Reflection() { }

void Reflection::addTo(EffectShaderBuilder& s) {
    std::string textureUniform = s.uniqueName("u_texture");
    std::string positionVarying = s.uniqueName("v_position");
    std::string factorUniform = s.uniqueName("u_factor");
    s.addTextureUniform("sampler2D", textureUniform);
    s.beginFragmentUniformBuffer(s.uniqueName("Reflection"));
    s.fInit << "float " << factorUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.requireLinearInterpolation();
    s.addVarying("vec2", positionVarying, "noperspective ");
    std::string color = s.uniqueName("color");
    s.vMain << positionVarying << " = (" << EffectShader::SCREEN_POSITION << ".xy / " << EffectShader::SCREEN_POSITION << ".w + 1.0)/2.0;\n";
    s.fMain << Shader::FRAGMENT_COLOR << " += vec4(" << factorUniform << " * texture(" << textureUniform << "," << positionVarying << ").rgb, 0.0);\n";
}

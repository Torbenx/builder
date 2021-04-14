#include "3d/CommonEffects.hpp"

EffectBindings AmbientLight::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
    s.newContext();
    std::string colorUniform = s.uniqueName("u_color");
    data.handles[0] = s.addFragmentUniform("vec3", colorUniform);
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
    return data;
}

EffectBindings UniformColor::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
    s.newContext();
    std::string colorUniform = s.uniqueName("u_color");
    data.handles[0] = s.addFragmentUniform("vec3", colorUniform);
    s.fMain << Shader::FRAGMENT_COLOR << " += vec4(" << colorUniform << " * " << EffectShader::LIGHT_INTENSITY << ", 0.0);\n";
    return data;
}

EffectBindings TextureColor::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
    s.newContext();
    std::string texPositionAttrib = s.uniqueName("a_texPosition");
    std::string textureUniform = s.uniqueName("u_texture");
    std::string texPositionVarying = s.uniqueName("v_texPosition");
    data.handles[0] = s.addAttrib("vec2", texPositionAttrib);
    data.handles[1] = s.addFragmentUniform("sampler2D", textureUniform);
    data.textureSlot = s.nextTextureSlot();
    s.addVarying("vec2", texPositionVarying);
    s.vMain << texPositionVarying << " = " << texPositionAttrib << ";\n";
    s.fMain << Shader::FRAGMENT_COLOR << " += vec4(texture(" << textureUniform << ", " << texPositionVarying << ").rgb * " << EffectShader::LIGHT_INTENSITY << ", 0.0);\n";
    return data;
}

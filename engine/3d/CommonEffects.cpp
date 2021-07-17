#include "3d/CommonEffects.hpp"

void AmbientLight::addTo(EffectShaderBuilder& s) {
    s.beginEffect();
    std::string colorUniform = s.uniqueName("u_color");
    s.addFragmentUniform("vec3", colorUniform);
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
}

void UniformColor::addTo(EffectShaderBuilder& s) {
    s.beginEffect();
    std::string colorUniform = s.uniqueName("u_color");
    s.addFragmentUniform("vec3", colorUniform);
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += " << colorUniform << ";\n";
}

void TextureColor::addTo(EffectShaderBuilder& s) {
    s.beginEffect();
    std::string texPositionAttrib = s.uniqueName("a_texPosition");
    std::string textureUniform = s.uniqueName("u_texture");
    std::string texPositionVarying = s.uniqueName("v_texPosition");
    s.addAttrib("vec2", texPositionAttrib);
    s.addFragmentUniform("sampler2D", textureUniform);
    s.nextTextureSlot();
    s.addVarying("vec2", texPositionVarying);
    s.vMain << texPositionVarying << " = " << texPositionAttrib << ";\n";
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += texture(" << textureUniform << ", " << texPositionVarying << ").rgb;\n";
}

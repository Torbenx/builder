#include "3d/CommonEffects.hpp"
#include "graphics/helpers.hpp"

// AmbientLight

template<>
Effect::TypeData IN_SECTION(effect) EffectImpl<AmbientLight>::effectType = registerEffectType();
AmbientLight::AmbientLight() { }

void AmbientLight::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.beginFragmentUniformBuffer(s.uniqueName("AmbientLight"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
}

// UniformColor

template<>
Effect::TypeData IN_SECTION(effect) EffectImpl<UniformColor>::effectType = registerEffectType();
UniformColor::UniformColor() { }

void UniformColor::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.beginFragmentUniformBuffer(s.uniqueName("UniformColor"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += " << colorUniform << ";\n";
}

// TextureColor

template<>
Effect::TypeData IN_SECTION(effect) EffectImpl<TextureColor>::effectType = registerEffectType();
TextureColor::TextureColor() { }

void TextureColor::addTo(EffectShaderBuilder& s) {
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

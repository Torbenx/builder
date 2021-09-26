#include "3d/CommonEffects.hpp"
#include "graphics/helpers.hpp"

// AmbientLight

void AmbientLight::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.beginFragmentUniformBuffer(s.uniqueName("AmbientLight"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
}

// UniformColor

void UniformColor::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.beginFragmentUniformBuffer(s.uniqueName("UniformColor"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.endFragmentUniformBuffer();
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += " << colorUniform << ";\n";
}

// TextureColor

void TextureColor::addTo(EffectShaderBuilder& s) {
    std::string texPositionAttrib = s.uniqueName("a_texPosition");
    std::string textureUniform = s.uniqueName("u_texture");
    std::string texPositionVarying = s.uniqueName("v_texPosition");
    s.addAttrib("vec2", texPositionAttrib);
    s.addTextureUniform("sampler2D", textureUniform);
    s.addVarying("vec2", texPositionVarying);
    s.vMain << texPositionVarying << " = " << texPositionAttrib << ";\n";
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += texture(" << textureUniform << ", " << texPositionVarying << ").rgb;\n";
}

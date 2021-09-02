#include "3d/CommonEffects.hpp"
#include "graphics/helpers.hpp"

// AmbientLight

template<>
const Effect::Type EffectImpl<AmbientLight>::effectType = registerEffectType();

AmbientLight::AmbientLight(Vec3 color, GAllocator& allocator)
    : color(color) {
    StaticUniformHelper<FragmentUniformBuffer> w;
    w.update(allocator).color = color;
    fragmentUniformBuffer = w.ptr;
}

void AmbientLight::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.beginFragmentUniformBuffer(s.uniqueName("AmbientLight"));
    s.fInit << "vec3 " << colorUniform << ";\n";
    s.endFragmentUniformBuffer();
    //s.addFragmentUniform("vec3", colorUniform);
    s.fMain << EffectShader::LIGHT_INTENSITY << " += " << colorUniform << ";\n";
}

AmbientLight::~AmbientLight() {
    //allocator.free(&fragmentUniformBuffer);
}

// UniformColor

template<>
const Effect::Type EffectImpl<UniformColor>::effectType = registerEffectType();

UniformColor::UniformColor(Vec3 color)
    : color(color) { }

void UniformColor::addTo(EffectShaderBuilder& s) {
    std::string colorUniform = s.uniqueName("u_color");
    s.addFragmentUniform("vec3", colorUniform);
    s.fMain << EffectShader::FRAGMENT_DIFFUSE_COLOR << " += " << colorUniform << ";\n";
}

// TextureColor

template<>
const Effect::Type EffectImpl<TextureColor>::effectType = registerEffectType();

TextureColor::TextureColor(Texture texture, VertexBuffer texCoords)
    : texture(texture), texCoords(texCoords) { }

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

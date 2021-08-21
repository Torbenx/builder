#include "3d/Reflection.hpp"

template<>
const Effect::Type EffectImpl<Reflection>::effectType = registerEffectType();

Reflection::Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor)
    : Renderpass(&framebuffer)
    , texture(Texture::color(width, height, Texture::Format::RGBA8888))
    , depthAttachment(Texture::depth(width, height, Texture::DepthBits::D16, true))
    , reflection(reflection)
    , position(position)
    , factor(factor) {

    framebuffer.bind();
    framebuffer.attach(texture);
    framebuffer.attach(depthAttachment);
    framebuffer.viewport(width, height);
}

void Reflection::updateViewProjectionMatrix(Mat4x4& viewProjectionMatrix) {
    this->viewProjectionMatrix = viewProjectionMatrix * getReflectionMatrix();
}

Mat4x4 Reflection::getReflectionMatrix() {
    return Mat4x4::reflection(reflection, position);
}

void Reflection::addTo(EffectShaderBuilder& s) {
    std::string textureUniform = s.uniqueName("u_texture");
    std::string positionVarying = s.uniqueName("v_position");
    std::string factorUniform = s.uniqueName("u_factor");
    s.requireLinearInterpolation();
    s.addFragmentUniform("sampler2D", textureUniform);
    s.nextTextureSlot();
    s.addFragmentUniform("float", factorUniform);
    s.addVarying("vec2", positionVarying, "noperspective ");
    std::string color = s.uniqueName("color");
    s.vMain << positionVarying << " = (" << EffectShader::SCREEN_POSITION << ".xy / " << EffectShader::SCREEN_POSITION << ".w + 1.0)/2.0;\n";
    s.fMain << Shader::FRAGMENT_COLOR << " += vec4(" << factorUniform << " * texture(" << textureUniform << "," << positionVarying << ").rgb, 0.0);\n";
}

void Reflection::applyTo(OrderedShader s) {
    s.uniformTexture(texture);
    s.uniform1f(factor);
}

#include "3d/Reflection.hpp"

Reflection::Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor)
    : EffectInstance(&EFFECT)
    , texture(Texture::color(width, height, Texture::Format::RGBA8888))
    , depthAttachment(Texture::depth(width, height, Texture::DepthBits::D16, true))
    , reflection(reflection)
    , position(position)
    , factor(factor) {

    framebuffer.attach(texture);
    framebuffer.attach(depthAttachment);
    framebuffer.viewport(width, height);
}

Mat4x4 Reflection::getReflectionMatrix() {
    return Mat4x4::reflection(reflection, position);
}

EffectBindings Reflection::Effect::addTo(EffectShaderBuilder& s) {
    EffectBindings data;
    s.newContext();
    std::string textureUniform = s.uniqueName("u_texture");
    std::string positionVarying = s.uniqueName("v_position");
    std::string factorUniform = s.uniqueName("u_factor");
    s.requireLinearInterpolation();
    s.addVarying("vec2", positionVarying, "noperspective ");
    data.handles[0] = s.addFragmentUniform("sampler2D", textureUniform);
    data.handles[1] = s.addFragmentUniform("float", factorUniform);
    data.textureSlot = s.nextTextureSlot();
    std::string color = s.uniqueName("color");
    s.vMain << positionVarying << " = (" << EffectShader::SCREEN_POSITION << ".xy / " << EffectShader::SCREEN_POSITION << ".w + 1.0)/2.0;\n";
    s.fMain << "vec3 " << color << " = " << factorUniform << " * texture(" << textureUniform << "," << positionVarying << ").rgb;\n";
    //s.fragmentColor = color;
    return data;
}

void Reflection::applyTo(EffectShader& s, const EffectBindings& data) {
    s.uniformTexture(data.handles[0], texture, data.textureSlot);
    s.uniform1f(data.handles[1], factor);
}

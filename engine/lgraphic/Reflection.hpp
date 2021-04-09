#pragma once

#include "GL/Framebuffer.hpp"
#include "lgraphic/LShader.hpp"

class Reflection : public LShader::Effect {
public:
    Framebuffer framebuffer;
    Texture depthAttachment;
    Texture texture;
    Vec3 reflection, position;
    float factor;
    Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor);
    Mat4x4 getReflectionMatrix();
    static LShader::EffectLocationHandle addTo(LShaderBuilder& s);
    virtual void bindTo(LShader&, const LShader::EffectData&) override;
    virtual std::string effectName() override { return "Reflection"; };
    virtual std::size_t hash() override { return std::hash<std::string>{}(Reflection::effectName()); }
};

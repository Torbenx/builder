#pragma once

#include "GL/Framebuffer.hpp"
#include "3d/Effect.hpp"
#include "resource/Resource.hpp"

class Reflection : public EffectInstance {
public:
    static inline class Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;
    Framebuffer framebuffer;
    Resource<Texture> depthAttachment;
    Resource<Texture> texture;
    Vec3 reflection, position;
    float factor;
    Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor);
    Mat4x4 getReflectionMatrix();
    virtual void applyTo(OrderedShader) override;
};

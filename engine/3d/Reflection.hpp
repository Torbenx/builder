#pragma once

#include "3d/Effect.hpp"
#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"
#include "resource/Resource.hpp"

class Reflection : public EffectInstance, public Renderpass {
public:
    static inline struct Effect : public ::Effect {
        Effect()
            : ::Effect(&Effect::addTo) { }
        static void addTo(EffectShaderBuilder&);
    } EFFECT;
    Framebuffer framebuffer;
    Resource<Texture> texture;
    Resource<Texture> depthAttachment;
    Vec3 reflection, position;
    float factor;
    Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor);
    Mat4x4 getReflectionMatrix();
    void updateViewProjectionMatrix(Mat4x4&);
    virtual void applyTo(OrderedShader) override;
};

#pragma once

#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"

class SpotLight : public EffectInstance, public DepthRenderpass {
public:
    static inline struct Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Resource<Texture> depthMap;

    SpotLight(Vec3 position, Vec3 direction, float fov, int width, int height, Vec3 color, float near, float far);
    SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int texWidth, int texHeight, Vec3 color, float near, float far);
    virtual void applyTo(OrderedShader) override;

private:
    Framebuffer framebuffer;
    Mat4x4 projection, view;
    Vec3 color;
    Vec3 direction, position, hDirection;
    float hfov, vfov, m_near, m_far;
    int width, height;
};

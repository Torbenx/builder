#pragma once

#include "3d/Scene.hpp"

class SpotLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Resource<Texture> depthMap;

    SpotLight(Vec3 position, Vec3 direction, float fov, int width, int height, Vec3 color, float near, float far);
    SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int texWidth, int texHeight, Vec3 color, float near, float far);
    virtual void applyTo(EffectShader&, const EffectBindings&) override;
    void addTo(Scene& scene);

private:
    Framebuffer framebuffer;
    Mat4x4 projection, view;
    Vec3 color;
    Vec3 direction, position, hDirection;
    float hfov, vfov, m_near, m_far;
    int width, height;
};

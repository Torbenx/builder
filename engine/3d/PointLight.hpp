#pragma once

#include "3d/Scene.hpp"
#include "GL/CubeMap.hpp"

class PointLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;
    Resource<CubeMap> depthMap;
    Framebuffer framebuffers[6];
    Vec3 position;
    Vec3 color;
    float m_near, m_far;
    int width, height;
    PointLight(Vec3 position, int width, int height, Vec3 color, float near, float far);
    virtual void applyTo(OrderedShader) override;
    void addTo(Scene& scene);
};

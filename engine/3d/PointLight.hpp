#pragma once

#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"

class PointLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        Effect()
            : ::Effect(&Effect::addTo) { }
        static void addTo(EffectShaderBuilder&);
    } EFFECT;
    Resource<CubeMap> depthMap;
    Framebuffer framebuffers[6];
    DepthRenderpass passes[6];
    Vec3 position;
    Vec3 color;
    float m_near, m_far;
    int width, height;
    PointLight(Vec3 position, int width, int height, Vec3 color, float near, float far);
    virtual void applyTo(OrderedShader) override;
    void addObject(id_t);
    void draw(GraphicsObjectCollection&);
    void updateViewProjectionMatrix();
};

#pragma once

#include "3d/Effect.hpp"
#include "GL/CubeMap.hpp"

class PointLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder&) override;
    } EFFECT;
    Vec3 position;
    Vec3 color;
    CubeMap depthMap;
    float m_near, m_far;
    PointLight(Vec3 position, int width, int height, Vec3 color, float near, float far);
    virtual void applyTo(EffectShader&, const EffectBindings&) override;
    Mat4x4 viewProjectionMatrix(CubeMap::Face);
};

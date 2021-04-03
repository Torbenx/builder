#pragma once

#include "lgraphic/DepthMap.hpp"
#include "lgraphic/LShader.hpp"

class SpotLight : public LShader::Effect{
public:
    DepthMap depthMap;
    SpotLight(Vec3 position, Vec3 direction, float fov, int texWidth, int texHeight, Vec3 color, float near, float far);
    SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int texWidth, int texHeight, Vec3 color, float near, float far);
    void draw(Renderable **ptrArray, int count);
    void addTo(LShaderBuilder&);
    virtual void apply(LShader&, const  LShader::EffectData&) override;
private:
    Mat4x4 projection, view;
    Vec3 color;
    Vec3 direction, position, hDirection;
    float hfov, vfov, m_near, m_far;
};

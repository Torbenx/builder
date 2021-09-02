#pragma once

#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"

class SpotLight : public EffectImpl<SpotLight>, public DepthRenderpass {
public:
    static void addTo(EffectShaderBuilder&);

    union {
        Texture depthMap;
        Texture textures[1];
    };

    SpotLight(Vec3 position, Vec3 direction, float fov, int width, int height, Vec3 color, float near, float far);
    SpotLight(Vec3 position, Vec3 direction, float hfov, float vfov, Vec3 hDirection, int texWidth, int texHeight, Vec3 color, float near, float far);
    void applyTo(OrderedShader) override;
    ~SpotLight();

private:
    Framebuffer framebuffer;
    Mat4x4 projection, view;
    Vec3 color;
    Vec3 direction, position, hDirection;
    float hfov, vfov, m_near, m_far;
    int width, height;
};

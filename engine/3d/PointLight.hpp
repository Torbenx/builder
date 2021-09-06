#pragma once

#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"

class PointLight : public EffectImpl<PointLight> {
public:
    union {
        CubeMap depthMap;
        Texture textures[1];
    };

    struct FragmentUniform {
        std140::vec3 color;
        std140::vec3 lightPos;
        float a;
        float b;
    };
    UniformHelper<FragmentUniform> fragmentUniform;

    static void addTo(EffectShaderBuilder&);
    PointLight();
};

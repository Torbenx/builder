#pragma once

#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"

class SpotLight : public EffectImpl<SpotLight> {
public:
    union {
        Texture depthMap;
        Texture textures[1];
    };

    struct FragmentUniform {
        std140::vec3 color;
        std140::vec3 lightPos;
    };
    UniformHelper<FragmentUniform> fragmentUniform;

    struct VertexUniform {
        std140::mat4 mvpMatrix;
    };
    UniformHelper<VertexUniform> vertexUniform;

    SpotLight();
    ~SpotLight([[context]] GAllocator* allocator) { }
    static void addTo(EffectShaderBuilder&);
};

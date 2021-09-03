#pragma once

#include "3d/Effect.hpp"
#include "3d/Renderpass.hpp"
#include "GL/Framebuffer.hpp"
#include "resource/Resource.hpp"

class Reflection : public EffectImpl<Reflection> {
public:
    union {
        Texture texture;
        Texture textures[1];
    };

    struct alignas(16) FragmentUniform {
        float factor;
    };
    TypedGPtr<FragmentUniform>* fragmentUniform;

    Reflection();
    static void addTo(EffectShaderBuilder&);
};

#pragma once

#include "3d/Effect.hpp"
#include "GL/std140.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectImpl<AmbientLight> {
public:
    struct FragmentUniform {
        std140::vec3 color;
    };
    UniformHelper<FragmentUniform> fragmentUniform;


    static void addTo(EffectShaderBuilder&);
};

class UniformColor : public EffectImpl<UniformColor> {
public:
    struct FragmentUniform {
        std140::vec3 color;
    };
    UniformHelper<FragmentUniform> fragmentUniform;


    static void addTo(EffectShaderBuilder&);
};

class TextureColor : public EffectImpl<TextureColor> {
public:
    union {
        Texture texture;
        Texture textures[1];
    };

    union {
        GPtr texCoords;
        GPtr vertexBuffers[1];
    };
    static inline const int_t vertexBufferElementSizes[] = { 2 };

    TextureColor()
        : texture(), texCoords() { }
    static void addTo(EffectShaderBuilder&);
};

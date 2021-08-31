#pragma once

#include "3d/Effect.hpp"
#include "GL/std140.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectImpl<AmbientLight> {
public:
    struct FragmentUniformBuffer {
        std140::vec3 color;
    };
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    GPtr fragmentUniformBuffer;
    AmbientLight(Vec3 color);
    ~AmbientLight();

    void applyTo(OrderedShader) override {
        //s.uniformVec3(color);
    }
};

class UniformColor : public EffectImpl<UniformColor> {
public:
    struct FragmentUniformBuffer {
        std140::vec3 color;
    };
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    UniformColor(Vec3 color);

    void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class TextureColor : public EffectImpl<TextureColor> {
public:
    static void addTo(EffectShaderBuilder&);

    union {
        const Texture texture;
        const Texture textures[1];
    };

    union {
        const VertexBuffer texCoords;
        const VertexBuffer vertexBuffers[1];
    };

    TextureColor(Texture texture, VertexBuffer texCoords);

    void applyTo(OrderedShader) override { }
};

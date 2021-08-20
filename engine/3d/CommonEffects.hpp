#pragma once

#include "3d/Effect.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectImpl<AmbientLight> {
public:
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    AmbientLight(Vec3 color);

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class UniformColor : public EffectImpl<UniformColor> {
public:
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    UniformColor(Vec3 color);

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class TextureColor : public EffectImpl<TextureColor> {
public:
    static void addTo(EffectShaderBuilder&);

    Texture texture;
    static inline const char* const vertexBufferNames[] = { "texCoords" };
    union {
        const VertexBuffer texCoords;
        const VertexBuffer vertexBuffers[1];
    };

    TextureColor(Texture texture, VertexBuffer texCoords);

    virtual void applyTo(OrderedShader s) override {
        s.attribPointer(texCoords);
        s.uniformTexture(texture);
    }
};

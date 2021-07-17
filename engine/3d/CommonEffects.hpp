#pragma once

#include "3d/Effect.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public Effect {
public:
    static inline const Type effectType = registerEffect<AmbientLight>();
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    AmbientLight(Vec3 color)
        : color(color) { }

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class UniformColor : public Effect {
public:
    static inline const Type effectType = registerEffect<UniformColor>();
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    UniformColor(Vec3 color)
        : color(color) { }

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class TextureColor : public Effect {
public:
    static inline const Type effectType = registerEffect<TextureColor>();
    static void addTo(EffectShaderBuilder&);

    Texture texture;
    VertexBuffer texCoords;
    TextureColor(Texture texture, VertexBuffer texCoords)
        : texture(texture), texCoords(texCoords) { }

    virtual void applyTo(OrderedShader s) override {
        s.attribPointer(texCoords);
        s.uniformTexture(texture);
    }
};

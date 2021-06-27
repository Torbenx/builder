#pragma once

#include "3d/Effect.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Vec3 color;
    AmbientLight(Vec3 color)
        : EffectInstance(&EFFECT), color(color) { }

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class UniformColor : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Vec3 color;
    UniformColor(Vec3 color)
        : EffectInstance(&EFFECT), color(color) { }

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class TextureColor : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual void addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Texture texture;
    VertexBuffer texCoords;
    TextureColor(Texture texture, VertexBuffer texCoords)
        : EffectInstance(&EFFECT), texture(texture), texCoords(texCoords) { }

    virtual void applyTo(OrderedShader s) override {
        s.attribPointer(texCoords);
        s.uniformTexture(texture);
    }
};

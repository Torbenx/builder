#pragma once

#include "3d/Effect.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Vec3 color;
    AmbientLight(Vec3 color)
        : EffectInstance(&EFFECT), color(color) { }

    virtual void applyTo(EffectShader& s, const EffectBindings& data) override {
        s.uniformVec3(data.handles[0], color);
    }
};

class UniformColor : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Vec3 color;
    UniformColor(Vec3 color)
        : EffectInstance(&EFFECT), color(color) { }

    virtual void applyTo(EffectShader& s, const EffectBindings& data) override {
        s.uniformVec3(data.handles[0], color);
    }
};

class TextureColor : public EffectInstance {
public:
    static inline struct Effect : public ::Effect {
        virtual EffectBindings addTo(EffectShaderBuilder&) override;
    } EFFECT;

    Texture texture;
    VertexBuffer texCoords;
    TextureColor(Texture texture, VertexBuffer texCoords)
        : EffectInstance(&EFFECT), texture(texture), texCoords(texCoords) { }

    virtual void applyTo(EffectShader& s, const EffectBindings& data) override {
        s.attribPointer(data.handles[0], texCoords);
        s.uniformTexture(data.handles[1], texture, data.textureSlot);
    }
};

#pragma once

#include "3d/Effect.hpp"
#include "animation/CommonProperties.hpp"
#include "math/Vec4.hpp"

class AmbientLight : public EffectImpl<AmbientLight> {
public:
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    AmbientLight(Vec3 color);

    Color col;
    static const BufferEntryDescription fragmentUniform[];

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class UniformColor : public EffectImpl<UniformColor> {
public:
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    UniformColor(Vec3 color);

    Color col;
    static const BufferEntryDescription fragmentUniform[];

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class TextureColor : public EffectImpl<TextureColor> {
public:
    static void addTo(EffectShaderBuilder&);

    static inline const TextureDescription textureDescriptions[] = { { "u_texture", TextureDescription::SamplerType::Sampler2D } };
    union {
        const Texture texture;
        const Texture textures[1];
    };

    static inline const VertexBufferDescription vertexBufferDescriptions[] = { { "a_texPosition", Property::Format::Vec2 } };
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

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
    UniformBufferDestination fragmentUniformDestination;
    AmbientLight(Vec3 color);

    virtual void applyTo(OrderedShader s) override {
        s.uniformVec3(color);
    }
};

class UniformColor : public EffectImpl<UniformColor> {
public:
    struct FragmentUniformBuffer {
        std140::vec3 color;
    };
    static void addTo(EffectShaderBuilder&);

    Vec3 color;
    UniformBufferDestination fragmentUniformDestination;
    UniformColor(Vec3 color);

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

    static inline const VertexBufferDescription vertexBufferDescriptions[] = { { "a_texPosition", VertexBufferDescription::Format::Vec2 } };
    union {
        const VertexBuffer texCoords;
        const VertexBuffer vertexBuffers[1];
    };

    TextureColor(Texture texture, VertexBuffer texCoords);

    virtual void applyTo(OrderedShader s) override { }
};

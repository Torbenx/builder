#pragma once

#include "3d/Effect.hpp"
#include "GL/Framebuffer.hpp"
#include "GL/Shader.hpp"

struct DepthRenderable {
    virtual void depthDraw(Framebuffer&, const Shader&, Shader::LocationHandle positionAttrib) = 0;
    virtual ~DepthRenderable() { }
};

struct DepthRenderpass {
    struct Entry {
        DepthRenderable* renderable;
        Mat4x4 modelMatrix;
    };
    std::unique_ptr<Framebuffer> framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<Entry> entries;
};

struct Renderable {
    virtual void draw(Framebuffer&, const EffectShader&, std::vector<EffectBindings>&) = 0;
    virtual ~Renderable() { }
};

struct Lightpass {
    struct Entry {
        Renderable* renderable;
        Mat4x4 modelMatrix;
    };
    struct EffectAndBindings {
        EffectInstance* effect;
        EffectBindings bindings;
    };
    std::vector<EffectAndBindings> effects;
    std::vector<Entry> entries;
};

struct Shaderpass {
    EffectShader* shader;
    std::vector<Lightpass> passes;
    std::vector<EffectBindings> materialBindings;
};

struct Renderpass {
    Mat4x4 viewProjectionMatrix;
    std::vector<Shaderpass> passes;
};

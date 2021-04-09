#pragma once

#include "3d/Effect.hpp"
#include "GL/Framebuffer.hpp"
#include "GL/Shader.hpp"

struct DepthRenderable {
    virtual void depthDraw(Framebuffer&, Shader&, Shader::LocationHandle positionAttrib) = 0;
    virtual ~DepthRenderable() { }
};

struct DepthRenderpass {
    struct Entry {
        DepthRenderable* renderable;
        Mat4x4 modelMatrix;
    };
    Mat4x4 viewProjectionMatrix;
    std::vector<Entry> entries;
};

struct Renderable {
    virtual void draw(Framebuffer&, EffectShader&, const MaterialBindings&) = 0;
    virtual ~Renderable() { }
};

struct Lightpass {
    struct Entry {
        Renderable* renderable;
        Mat4x4 modelMatrix;
    };
    struct EffectBinding {
        EffectLocationHandle handle;
        EffectInstance* effect;
    };
    std::vector<EffectBinding> effects;
    std::vector<Entry> entries;
};

struct Shaderpass {
    EffectShader* shader;
    std::vector<Lightpass> passes;
    MaterialBindings materialBindings;
};

struct Renderpass {
    Mat4x4 viewProjectionMatrix;
    std::vector<Shaderpass> passes;
};

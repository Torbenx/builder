#pragma once

#include "3d/Shaderpass.hpp"

struct DepthRenderpass {
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<id_t> objectIds;
    Resource<SimpleDepthShader> depthShader = Shaders::depth();

    DepthRenderpass(Framebuffer* framebuffer)
        : framebuffer(framebuffer) { }

    void draw(GraphicsObjectCollection&, Context<GAllocator*> = {});
    void addObject(id_t objectId);
};

class Renderpass {
public:
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<std::unique_ptr<Shaderpass>> passes;

    Renderpass(Framebuffer* framebuffer)
        : framebuffer(framebuffer) { }

    void draw(GraphicsObjectCollection&, Context<GAllocator*> = {});
    void addObject(id_t objectId, std::vector<Effect::Handle> effects);

private:
    struct ObjectEntry {
        std::vector<Effect::Handle> effects;
        int_t index;
    };
    std::vector<int_t> shaderIndices;
    std::vector<id_t> objectIds;
    std::vector<ObjectEntry> objectEntries;
};

#pragma once

#include "3d/Effect.hpp"
#include "GL/Framebuffer.hpp"
#include "math/Box.hpp"
#include "math/solver/LinearProgramming.hpp"
#include "util/IdAllocator.hpp"
#include <set>
#include <unordered_map>

class Shaderpass;

struct Renderpass {
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<std::unique_ptr<Shaderpass>> passes;
};

struct DepthRenderpass {
    struct Entry {
        id_t objectId;
        Mat4x4 modelViewProjectionMatrix;
    };
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<Entry> entries;
};

struct SceneObject {
    Mat4x4 modelMatrix;
    DrawMode drawMode;
    VertexBuffer vertices;
    VertexBuffer normals;
    IndexBuffer indices;
    Box boundingBox;
};

struct Shaderpass {
    static std::unique_ptr<Shaderpass> construct(Resource<id_t> shaderHandle);

    virtual void draw(std::vector<SceneObject>&, Renderpass&) = 0;
    virtual void addEntry(int_t objectId, std::vector<EffectInstance*>& effects) = 0;
    virtual ~Shaderpass() { }
};

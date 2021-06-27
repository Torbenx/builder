#pragma once

#include "3d/Effect.hpp"
#include "3d/Shaderpass.hpp"
#include "GL/Framebuffer.hpp"
#include "math/Box.hpp"
#include "util/Shaders.hpp"
#include <unordered_map>
#include <vector>

class Scene {
public:
    struct Object {
        Mat4x4 modelMatrix;
        DrawMode drawMode;
        VertexBuffer vertices;
        VertexBuffer normals;
        IndexBuffer indices;
        Box boundingBox;
    };

    struct DepthRenderpass {
        struct Entry {
            int objectId;
            Mat4x4 modelViewProjectionMatrix;
        };
        Framebuffer* framebuffer;
        Mat4x4 viewProjectionMatrix;
        std::vector<Entry> entries;
    };

    static constexpr int MAX_RENDERPASSES = 5;

    ~Scene();
    void addEffect(EffectInstance*);
    void addObject(const Object& object, const std::vector<EffectInstance*>& effects);
    int addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addObjectToRenderpass(int renderpassIndex, int objectId);
    void updateViewProjectionMatrix(int renderpassIndex, const Mat4x4& viewProjectionMatrix) { renderpasses[renderpassIndex].viewProjectionMatrix = viewProjectionMatrix; }
    void draw();

    struct ObjectEntry {
        Object object;
        std::vector<EffectInstance*> effects;
    };
    struct ShaderEntry {
        EffectShader shader;
        int indices[MAX_RENDERPASSES];
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<ObjectEntry> objects;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpasses[MAX_RENDERPASSES];
    int renderpassCount = 0;
    std::vector<EffectInstance*> globalEffects;
    std::unordered_map<uintptr_t, int> shaderIds;
    std::vector<ShaderEntry> shaderEntries;
    IdAllocator<int> shaderIdAllocator;
};

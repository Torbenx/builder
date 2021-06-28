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
            id_t objectId;
            Mat4x4 modelViewProjectionMatrix;
        };
        Framebuffer* framebuffer;
        Mat4x4 viewProjectionMatrix;
        std::vector<Entry> entries;
    };

    static constexpr int_t MAX_RENDERPASSES = 5;

    ~Scene();
    void addEffect(EffectInstance*);
    void addObject(const Object& object, const std::vector<EffectInstance*>& effects);
    int_t addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addObjectToRenderpass(int_t renderpassIndex, id_t objectId);
    void updateViewProjectionMatrix(int_t renderpassIndex, const Mat4x4& viewProjectionMatrix) { renderpasses[renderpassIndex].viewProjectionMatrix = viewProjectionMatrix; }
    void draw();

    struct ObjectEntry {
        Object object;
        std::vector<EffectInstance*> effects;
    };
    struct ShaderEntry {
        int_t indices[MAX_RENDERPASSES];
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<ObjectEntry> objects;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpasses[MAX_RENDERPASSES];
    int_t renderpassCount = 0;
    std::vector<EffectInstance*> globalEffects;
    std::vector<ShaderEntry> shaderEntries;
};

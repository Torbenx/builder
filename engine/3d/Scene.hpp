#pragma once

#include "3d/Effect.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Shaders.hpp"
#include "math/Box.hpp"
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

    struct Shaderpass {
        struct EffectAndBindings {
            EffectInstance* effect;
            EffectBindings bindings;
        };
        struct Entry {
            int objectId;
            std::vector<EffectAndBindings> effects;
            Mat4x4 modelMatrix;
        };
        EffectShader* shader;
        std::vector<Entry> entries;
    };

    struct Renderpass {
        Framebuffer* framebuffer;
        Mat4x4 viewProjectionMatrix;
        std::vector<Shaderpass> passes;
    };

    static const int MAX_RENDERPASSES = 5;

    ~Scene();
    void addEffect(EffectInstance*);
    void addObject(const Object& object, const std::vector<EffectInstance*>& effects);
    int addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addObjectToRenderpass(int renderpassIndex, int objectId);
    void updateViewProjectionMatrix(int renderpassIndex, const Mat4x4& viewProjectionMatrix) { renderpasses[renderpassIndex].viewProjectionMatrix = viewProjectionMatrix; }
    void draw();

private:
    struct ObjectEntry {
        Object object;
        std::vector<EffectInstance*> effects;
    };
    struct ShaderEntry {
        int indices[MAX_RENDERPASSES];
        EffectShader* shader = nullptr;
        std::vector<EffectBindings> bindings;
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<ObjectEntry> objects;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpasses[MAX_RENDERPASSES];
    int renderpassCount = 0;
    std::vector<EffectInstance*> globalEffects;
    std::unordered_map<ptrdiff_t, ShaderEntry> shaders;
};

#pragma once

#include "3d/Effect.hpp"
#include "3d/Shaderpass.hpp"
#include "GL/Framebuffer.hpp"
#include "util/Shaders.hpp"
#include <unordered_map>
#include <vector>

class Scene {
public:
    static constexpr int_t MAX_RENDERPASSES = 5;

    ~Scene();
    void addEffect(EffectInstance*);
    void addObject(const SceneObject& object, const std::vector<EffectInstance*>& effects);
    int_t addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void addObjectToRenderpass(int_t renderpassIndex, id_t objectId);
    void updateViewProjectionMatrix(int_t renderpassIndex, const Mat4x4& viewProjectionMatrix) { renderpasses[renderpassIndex].viewProjectionMatrix = viewProjectionMatrix; }
    void draw();

    struct ObjectEntry {
        std::vector<EffectInstance*> effects;
    };
    struct ShaderEntry {
        int_t indices[MAX_RENDERPASSES];
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<SceneObject> objects;
    std::vector<ObjectEntry> objectEntries;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpasses[MAX_RENDERPASSES];
    int_t renderpassCount = 0;
    std::vector<EffectInstance*> globalEffects;
    std::vector<ShaderEntry> shaderEntries;
};

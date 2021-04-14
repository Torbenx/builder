#pragma once

#include "3d/Effect.hpp"
#include "GL/Framebuffer.hpp"
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
    };

    struct DepthRenderpass {
        struct Entry {
            int objectId;
            Mat4x4 modelMatrix;
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
        Mat4x4 viewProjectionMatrix;
        std::vector<Shaderpass> passes;
    };

    void addEffect(EffectInstance*);
    void addObject(const Object& object, const std::vector<EffectInstance*>& effects);
    void addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix);
    void setup();
    void draw(Mat4x4 vp);

private:
    struct ObjectEntry {
        Object object;
        std::vector<EffectInstance*> effects;
    };
    struct ShaderEntry {
        int index;
        EffectShader* shader = nullptr;
        std::vector<EffectBindings> bindings;
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<ObjectEntry> objects;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpass;
    std::vector<EffectInstance*> globalEffects;
    std::unordered_map<ptrdiff_t, ShaderEntry> shaders;
};

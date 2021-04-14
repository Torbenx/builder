#pragma once

#include "3d/CommonEffects.hpp"
#include "3d/PointLight.hpp"
#include "3d/Reflection.hpp"
#include "3d/Renderpass.hpp"
#include "3d/SpotLight.hpp"
#include "util/Shaders.hpp"
#include <list>
#include <memory>
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
        std::unique_ptr<Framebuffer> framebuffer;
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

    void addLight(PointLight& light);
    void addLight(SpotLight& light);
    void addLight(AmbientLight& light);
    void addReflection(Reflection& ref);
    void addObject(const Object& object, const std::vector<EffectInstance*>& effects);
    void setup();
    void draw(Mat4x4 vp);

private:
    struct ObjectEntry {
        Object object;
        std::vector<EffectInstance*> effects;
    };
    Resource<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<ObjectEntry> objects;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpass;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    std::vector<AmbientLight*> ambientLights;
    std::vector<Reflection*> reflections;
};

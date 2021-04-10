#pragma once

#include "3d/AmbientLight.hpp"
#include "3d/PointLight.hpp"
#include "3d/Reflection.hpp"
#include "3d/Renderpass.hpp"
#include "3d/SpotLight.hpp"
#include "util/Shaders.hpp"
#include <list>
#include <memory>
#include <vector>

struct SceneRenderable : public Renderable, public DepthRenderable {
    virtual std::vector<EffectBindings> addEffects(EffectShaderBuilder&) = 0;
};

class Scene {
public:
    void addLight(PointLight& light);
    void addLight(SpotLight& light);
    void addLight(AmbientLight& light);
    void addReflection(Reflection& ref);
    void addRenderable(SceneRenderable& renderable, Mat4x4& model = Mat4x4::Id);
    void setup();
    void draw(Mat4x4 vp);

private:
    struct RenderableEntry {
        SceneRenderable* renderable;
        Mat4x4 model;
    };
    std::shared_ptr<SimpleDepthShader> depthShader = Shaders::depth();
    std::vector<RenderableEntry> renderables;
    std::vector<DepthRenderpass> depthPasses;
    Renderpass renderpass;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    std::vector<AmbientLight*> ambientLights;
    std::vector<Reflection*> reflections;
};

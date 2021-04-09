#include "3d/Scene.hpp"

void Scene::addLight(PointLight& light) {
    pointLights.push_back(&light);
}

void Scene::addLight(SpotLight& light) {
    spotLights.push_back(&light);
}

void Scene::addLight(AmbientLight& light) {
    ambientLights.push_back(&light);
}

void Scene::addReflection(Reflection& ref) {
    reflections.push_back(&ref);
}

void Scene::addRenderable(SceneRenderable& renderable, Mat4x4& model) {
    renderables.push_back({ &renderable, model });
}

void Scene::setup() {
    for (SpotLight* l : spotLights) {
        DepthRenderpass pass;
        pass.viewProjectionMatrix = l->viewProjectionMatrix();
        pass.entries.resize(renderables.size());
        for (RenderableEntry& entry : renderables)
            pass.entries.push_back({ entry.renderable, entry.model });
        depthPasses.push_back(pass);
    }
    for (PointLight* l : pointLights) {
        for (auto face : CubeMap::Faces) {
            DepthRenderpass pass;
            pass.viewProjectionMatrix = l->viewProjectionMatrix(face);
            pass.entries.resize(renderables.size());
            for (RenderableEntry& entry : renderables)
                pass.entries.push_back({ entry.renderable, entry.model });
            depthPasses.push_back(pass);
        }
    }
    for (RenderableEntry& entry : renderables) {
        AttributeList attributes;
        EffectShaderBuilder builder(attributes);
        Lightpass lightpass;
        for (SpotLight* l : spotLights) {
            lightpass.effects.push_back({ l, l->effect->addTo(builder) });
        }
        for (PointLight* l : pointLights) {
            lightpass.effects.push_back({ l, l->effect->addTo(builder) });
        }
        for (AmbientLight* l : ambientLights) {
            lightpass.effects.push_back({ l, l->effect->addTo(builder) });
        }
        lightpass.entries.push_back({ entry.renderable, entry.model });
        Shaderpass shaderpass;
        shaderpass.passes.push_back(lightpass);
        shaderpass.materialBindings = entry.renderable->addEffects(builder);
        shaderpass.shader = new EffectShader(builder);
        renderpass.passes.push_back(shaderpass);
    }
}

void Scene::draw(Mat4x4 vp) {
    Framebuffer::DEFAULT.clear();
    renderpass.viewProjectionMatrix = vp;
    for (Shaderpass& shaderpass : renderpass.passes) {
        shaderpass.shader->use();
        shaderpass.shader->uniformMat4x4(shaderpass.shader->viewProjectionUniform, renderpass.viewProjectionMatrix);
        for (Lightpass& lightpass : shaderpass.passes) {
            for (auto& effect : lightpass.effects) {
                effect.effect->applyTo(*shaderpass.shader, effect.bindings);
            }
            for (auto& entry : lightpass.entries) {
                shaderpass.shader->uniformMat4x4(shaderpass.shader->modelUniform, entry.modelMatrix);
                entry.renderable->draw(Framebuffer::DEFAULT, *shaderpass.shader, shaderpass.materialBindings);
            }
        }
    }
}

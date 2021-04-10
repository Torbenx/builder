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
        pass.framebuffer = std::make_unique<Framebuffer>();
        pass.framebuffer->attach(l->depthMap);
        pass.framebuffer->viewport(2048, 2048);
        pass.viewProjectionMatrix = l->viewProjectionMatrix();
        pass.entries.reserve(renderables.size());
        for (RenderableEntry& entry : renderables)
            pass.entries.push_back({ entry.renderable, entry.model });
        depthPasses.push_back(std::move(pass));
    }
    for (PointLight* l : pointLights) {
        for (auto face : CubeMap::Faces) {
            DepthRenderpass pass;
            pass.framebuffer = std::make_unique<Framebuffer>();
            pass.framebuffer->attach(l->depthMap, face);
            pass.framebuffer->viewport(1024, 1024);
            pass.viewProjectionMatrix = l->viewProjectionMatrix(face);
            pass.entries.reserve(renderables.size());
            for (RenderableEntry& entry : renderables)
                pass.entries.push_back({ entry.renderable, entry.model });
            depthPasses.push_back(std::move(pass));
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
    depthShader->use();
    for(DepthRenderpass& pass : depthPasses){
        pass.framebuffer->bind();
        pass.framebuffer->clear();
        depthShader->uniformMat4x4(depthShader->mvpUniform, pass.viewProjectionMatrix);
        for(auto& entry : pass.entries) {
            depthShader->uniformMat4x4(depthShader->modelUniform, entry.modelMatrix);
            entry.renderable->depthDraw(*pass.framebuffer, *depthShader, depthShader->positionAttrib);
        }
    }

    Framebuffer::DEFAULT.bind();
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

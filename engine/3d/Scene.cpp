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

void Scene::addObject(const Object& object, const std::vector<EffectInstance*>& effects) {
    objects.push_back({ object, effects });
}

void Scene::setup() {
    for (SpotLight* l : spotLights) {
        DepthRenderpass pass;
        pass.framebuffer = std::make_unique<Framebuffer>();
        pass.framebuffer->attach(l->depthMap);
        pass.framebuffer->viewport(2048, 2048);
        pass.viewProjectionMatrix = l->viewProjectionMatrix();
        pass.entries.reserve(objects.size());
        for (int i = 0; i < objects.size(); i++)
            pass.entries.push_back({ i, objects[i].object.modelMatrix });
        depthPasses.push_back(std::move(pass));
    }
    for (PointLight* l : pointLights) {
        for (auto face : CubeMap::Faces) {
            DepthRenderpass pass;
            pass.framebuffer = std::make_unique<Framebuffer>();
            pass.framebuffer->attach(l->depthMap, face);
            pass.framebuffer->viewport(1024, 1024);
            pass.viewProjectionMatrix = l->viewProjectionMatrix(face);
            pass.entries.reserve(objects.size());
            for (int i = 0; i < objects.size(); i++)
                pass.entries.push_back({ i, objects[i].object.modelMatrix });
            depthPasses.push_back(std::move(pass));
        }
    }
    for (int i = 0; i < objects.size(); i++) {
        AttributeList attributes;
        EffectShaderBuilder builder(attributes);
        Shaderpass::Entry entry;
        entry.objectId = i;
        entry.modelMatrix = objects[i].object.modelMatrix;
        for (SpotLight* l : spotLights) {
            entry.effects.push_back({ l, l->effect->addTo(builder) });
        }
        for (PointLight* l : pointLights) {
            entry.effects.push_back({ l, l->effect->addTo(builder) });
        }
        for (AmbientLight* l : ambientLights) {
            entry.effects.push_back({ l, l->effect->addTo(builder) });
        }
        for (EffectInstance* effect : objects[i].effects) {
            entry.effects.push_back({ effect, effect->effect->addTo(builder) });
        }
        Shaderpass shaderpass;
        shaderpass.entries.push_back(entry);
        shaderpass.shader = new EffectShader(builder);
        renderpass.passes.push_back(shaderpass);
    }
}

void Scene::draw(Mat4x4 vp) {
    depthShader->use();
    for (DepthRenderpass& pass : depthPasses) {
        pass.framebuffer->bind();
        pass.framebuffer->clear();
        depthShader->uniformMat4x4(depthShader->mvpUniform, pass.viewProjectionMatrix);
        for (auto& entry : pass.entries) {
            depthShader->uniformMat4x4(depthShader->modelUniform, entry.modelMatrix);
            Object& object = objects[entry.objectId].object;
            depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
            pass.framebuffer->draw(object.drawMode, object.indices);
        }
    }

    Framebuffer::DEFAULT.bind();
    Framebuffer::DEFAULT.clear();
    renderpass.viewProjectionMatrix = vp;
    for (Shaderpass& shaderpass : renderpass.passes) {
        shaderpass.shader->use();
        shaderpass.shader->uniformMat4x4(shaderpass.shader->viewProjectionUniform, renderpass.viewProjectionMatrix);
        for (Shaderpass::Entry& entry : shaderpass.entries) {
            for (auto& effect : entry.effects) {
                effect.effect->applyTo(*shaderpass.shader, effect.bindings);
            }
            shaderpass.shader->uniformMat4x4(shaderpass.shader->modelUniform, entry.modelMatrix);
            Object& object = objects[entry.objectId].object;
            shaderpass.shader->attribPointer(shaderpass.shader->positionAttrib, object.vertices);
            shaderpass.shader->attribPointer(shaderpass.shader->normalAttrib, object.normals);
            Framebuffer::DEFAULT.draw(object.drawMode, object.indices);
        }
    }
}

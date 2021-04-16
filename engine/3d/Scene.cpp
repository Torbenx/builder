#include "3d/Scene.hpp"
#include "util/time.hpp"

Scene::~Scene() {
    for(auto pair : shaders) {
        pair.second.shader->release();
    }
}

void Scene::addEffect(EffectInstance* effect) {
    globalEffects.push_back(effect);
}

void Scene::addObject(const Object& object, const std::vector<EffectInstance*>& effects) {
    int objectId = objects.size();
    objects.push_back({ object, effects });
    for (DepthRenderpass& pass : depthPasses) {
        pass.entries.push_back({ objectId, pass.viewProjectionMatrix * object.modelMatrix });
    }
    for (int i = 0; i < renderpassCount; i++) {
        addObjectToRenderpass(i, objectId);
    }
}

void Scene::addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix) {
    DepthRenderpass pass;
    pass.framebuffer = framebuffer;
    pass.viewProjectionMatrix = viewProjectionMatrix;
    pass.entries.reserve(objects.size());
    for (int i = 0; i < objects.size(); i++)
        pass.entries.push_back({ i, pass.viewProjectionMatrix * objects[i].object.modelMatrix });
    depthPasses.push_back(std::move(pass));
}

int Scene::addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix) {
    int index = renderpassCount++;
    Renderpass& renderpass = renderpasses[index];
    renderpass.framebuffer = framebuffer;
    renderpass.viewProjectionMatrix = viewProjectionMatrix;
    return index;
}

void Scene::addObjectToRenderpass(int renderpassIndex, int objectId) {
    Renderpass& renderpass = renderpasses[renderpassIndex];
    Shaderpass::Entry entry;
    entry.objectId = objectId;
    entry.modelMatrix = objects[objectId].object.modelMatrix;
    std::vector<EffectInstance*> effects;
    effects.reserve(globalEffects.size() * objects[objectId].effects.size());
    effects.insert(effects.end(), globalEffects.begin(), globalEffects.end());
    effects.insert(effects.end(), objects[objectId].effects.begin(), objects[objectId].effects.end());
    std::sort(effects.begin(), effects.end());
    ptrdiff_t hash = 0;
    for (EffectInstance* instance : effects) {
        hash ^= ((ptrdiff_t)instance->effect) >> 3;
    }
    // LOGD("hash: %llx", hash);
    ShaderEntry& shader = shaders[hash];
    if (!shader.shader) {
        AttributeList attributes;
        EffectShaderBuilder builder(attributes);
        for (EffectInstance* effect : effects) {
            shader.bindings.push_back(effect->effect->addTo(builder));
        }
        shader.shader = new EffectShader(builder);
        for (int i = 0; i < MAX_RENDERPASSES; i++) {
            shader.indices[i] = -1;
        }
    }
    Shaderpass* shaderpass;
    int shaderpassIndex = shader.indices[renderpassIndex];
    if (shaderpassIndex == -1) {
        shaderpassIndex = renderpass.passes.size();
        shader.indices[renderpassIndex] = shaderpassIndex;
        shaderpass = &renderpass.passes.emplace_back();
        shaderpass->shader = shader.shader;
    } else {
        shaderpass = &renderpass.passes[shaderpassIndex];
    }

    for (int i = 0; i < effects.size(); i++) {
        entry.effects.push_back({ effects[i], shader.bindings[i] });
    }
    shaderpass->entries.push_back(entry);
}

void Scene::draw() {
    uint64_t start = getNanos();
    depthShader->use();
    for (DepthRenderpass& pass : depthPasses) {
        pass.framebuffer->bind();
        pass.framebuffer->clear();
        for (auto& entry : pass.entries) {
            depthShader->uniformMat4x4(depthShader->mvpUniform, entry.modelViewProjectionMatrix);
            Object& object = objects[entry.objectId].object;
            depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
            pass.framebuffer->draw(object.drawMode, object.indices);
        }
    }

    for (int i = renderpassCount - 1; i >= 0; i--) {
        Renderpass& renderpass = renderpasses[i];
        renderpass.framebuffer->bind();
        renderpass.framebuffer->clear();
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
                renderpass.framebuffer->draw(object.drawMode, object.indices);
            }
        }
    }
    uint64_t end = getNanos();
    //LOGD("draw took %f us", (end - start) / 1000.0);
}

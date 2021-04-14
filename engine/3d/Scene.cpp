#include "3d/Scene.hpp"
#include "util/time.hpp"

void Scene::addEffect(EffectInstance* effect) {
    globalEffects.push_back(effect);
}

void Scene::addObject(const Object& object, const std::vector<EffectInstance*>& effects) {
    for (DepthRenderpass& pass : depthPasses) {
        pass.entries.push_back({ static_cast<int>(objects.size()), object.modelMatrix });
    }
    objects.push_back({ object, effects });
}

void Scene::addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix) {
    DepthRenderpass pass;
    pass.framebuffer = framebuffer;
    pass.viewProjectionMatrix = viewProjectionMatrix;
    pass.entries.reserve(objects.size());
    for (int i = 0; i < objects.size(); i++)
        pass.entries.push_back({ i, objects[i].object.modelMatrix });
    depthPasses.push_back(std::move(pass));
}

void Scene::setup() {
    for (int i = 0; i < objects.size(); i++) {
        Shaderpass::Entry entry;
        entry.objectId = i;
        entry.modelMatrix = objects[i].object.modelMatrix;
        std::vector<EffectInstance*> effects;
        effects.reserve(globalEffects.size() * objects[i].effects.size());
        effects.insert(effects.end(), globalEffects.begin(), globalEffects.end());
        effects.insert(effects.end(), objects[i].effects.begin(), objects[i].effects.end());
        std::sort(effects.begin(), effects.end());
        ptrdiff_t hash = 0;
        for (EffectInstance* instance : effects) {
            hash ^= ((ptrdiff_t)instance->effect) >> 3;
        }
        LOGD("hash: %llx", hash);
        ShaderEntry& shader = shaders[hash];
        Shaderpass *shaderpass;

        if (!shader.shader) {
            AttributeList attributes;
            EffectShaderBuilder builder(attributes);
            for (EffectInstance* effect : effects) {
                shader.bindings.push_back(effect->effect->addTo(builder));
            }
            shader.shader = new EffectShader(builder);
            shader.index = renderpass.passes.size();
            shaderpass = &renderpass.passes.emplace_back();
            shaderpass->shader = shader.shader;
        } else {
            shaderpass = &renderpass.passes[shader.index];
        }

        for (int i=0;i<effects.size();i++) {
            entry.effects.push_back({ effects[i], shader.bindings[i] });
        }
        shaderpass->entries.push_back(entry);
    }
}

void Scene::draw(Mat4x4 vp) {
    static bool first = true;
    if(first){
        first = false;
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
    }

    Framebuffer::DEFAULT.bind();
    Framebuffer::DEFAULT.clear();
    renderpass.viewProjectionMatrix = vp;
    uint64_t start = getNanos();
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
    uint64_t end = getNanos();
    //LOGD("draw took %f us", (end - start) / 1000.0);
}

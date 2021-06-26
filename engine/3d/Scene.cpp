#include "3d/Scene.hpp"
#include "math/solver/LinearProgramming.hpp"
#include "util/LogTimer.hpp"
#include <unordered_set>

Scene::~Scene() {
    for (auto pair : shaderIds) {
        shaderEntries[pair.second].shader.release();
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

    std::vector<EffectInstance*> effects;
    effects.reserve(globalEffects.size() + objects[objectId].effects.size());
    effects.insert(effects.end(), globalEffects.begin(), globalEffects.end());
    effects.insert(effects.end(), objects[objectId].effects.begin(), objects[objectId].effects.end());
    std::sort(effects.begin(), effects.end(), [](EffectInstance* first, EffectInstance* second) {
        return first->effect < second->effect;
    });
    uintptr_t hash = 0;
    for (EffectInstance* instance : effects) {
        hash ^= ((uintptr_t)instance->effect) >> 3;
    }

    /*LOGD("hash: %llx", hash);
    for (EffectInstance* instance : effects) {
        LOGD("0x%p: 0x%p", instance, instance->effect);
    }
    LOGD("");*/

    auto it = shaderIds.find(hash);
    int shaderId;
    if (it == shaderIds.end()) {
        shaderId = shaderIdAllocator.allocateId();
        shaderIds.insert({ hash, shaderId });
        if (shaderId == shaderEntries.size()) {
            shaderEntries.resize(shaderId + 1);
        }
        ShaderEntry& shader = shaderEntries[shaderId];
        AttributeList attributes;
        EffectShaderBuilder builder(attributes);
        for (EffectInstance* effect : effects) {
            shader.bindings.push_back(effect->effect->addTo(builder));
        }
        shader.shader.initialize(builder);
        for (int i = 0; i < MAX_RENDERPASSES; i++) {
            shader.indices[i] = -1;
        }
    } else {
        shaderId = it->second;
    }

    ShaderEntry& shader = shaderEntries[shaderId];
    Shaderpass* shaderpass;
    int shaderpassIndex = shader.indices[renderpassIndex];
    if (shaderpassIndex == -1) {
        shaderpassIndex = renderpass.passes.size();
        shader.indices[renderpassIndex] = shaderpassIndex;
        shaderpass = &renderpass.passes.emplace_back(shaderId, shader.bindings.size());
    } else {
        shaderpass = &renderpass.passes[shaderpassIndex];
    }

    shaderpass->addEntry(objectId, effects);
}

void Scene::draw() {
    //LinearProgramming<BasicZeroTester> lp;

    /*uint32_t k = 4;
    lp.n = 4;
    lp.numConstraints = 200;
    lp.numVariables = k * lp.numConstraints + lp.n - k;
    lp.constraints = new uint32_t[lp.n * lp.numVariables];
    for (uint32_t i = 0; i < lp.numConstraints; i++) {
        for (uint32_t j = 0; j < lp.n; j++) {
            if(j < k)
                lp.constraints[i * lp.n + j] = i * k + j;
            else
                lp.constraints[i * lp.n + j] = lp.numVariables - lp.n + j;
        }
    }*/

    /*uint32_t constraints[] = {
        0, 3, 4, 5,
        1, 3, 4, 5,
        2, 3, 4, 5
    };*/
    /*uint32_t constraints[] = {
        1, 2, 3, 4,
        0, 2, 3, 4,
        0, 1, 3, 4,
        1, 5, 6, 8,
        2, 3, 5, 7,
        2, 9, 10, 11,
    };
    lp.n = 4;
    lp.numConstraints = 6;
    lp.numVariables = 12;
    lp.constraints = constraints;*/
    /*{
        LogTimer timer;
        lp.doStuff();
    }*/
    //lp.basis.debug();
    //lp.dual.debug();

    auto isVisable = [](const Mat4x4& mvp, const Object& object) {
        Vec4 base = mvp * Vec4(object.boundingBox.left, object.boundingBox.bottom, object.boundingBox.front, 1.0f);
        Vec4 px = mvp * Vec4(object.boundingBox.right, object.boundingBox.bottom, object.boundingBox.front, 1.0f);
        Vec4 py = mvp * Vec4(object.boundingBox.left, object.boundingBox.top, object.boundingBox.front, 1.0f);
        Vec4 pz = mvp * Vec4(object.boundingBox.left, object.boundingBox.bottom, object.boundingBox.back, 1.0f);
        base.clampToW();
        px.clampToW();
        py.clampToW();
        pz.clampToW();
        float volume = Vec3::dot((px - base).xyz(), Vec3::cross((py - base).xyz(), (pz - base).xyz()));
        return std::abs(volume) > 0.0001f || std::isnan(volume);
    };

    depthShader->use();
    for (DepthRenderpass& pass : depthPasses) {
        pass.framebuffer->bind();
        pass.framebuffer->clear();
        for (auto& entry : pass.entries) {
            Object& object = objects[entry.objectId].object;
            if (!isVisable(entry.modelViewProjectionMatrix, object))
                continue;
            depthShader->uniformMat4x4(depthShader->mvpUniform, entry.modelViewProjectionMatrix);
            depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
            pass.framebuffer->draw(object.drawMode, object.indices);
        }
    }

    for (int i = renderpassCount - 1; i >= 0; i--) {
        Renderpass& renderpass = renderpasses[i];
        renderpass.framebuffer->bind();
        renderpass.framebuffer->clear();
        int rendererObjects = 0;
        for (Shaderpass& shaderpass : renderpass.passes) {
            shaderpass.draw(*this, renderpass);
        }
    }
}

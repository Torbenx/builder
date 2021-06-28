#include "3d/Scene.hpp"
#include "math/solver/LinearProgramming.hpp"
#include "util/LogTimer.hpp"
#include <unordered_set>

Scene::~Scene() {

}

void Scene::addEffect(EffectInstance* effect) {
    globalEffects.push_back(effect);
}

void Scene::addObject(const Object& object, const std::vector<EffectInstance*>& effects) {
    id_t objectId = objects.size();
    objects.push_back({ object, effects });
    for (DepthRenderpass& pass : depthPasses) {
        pass.entries.push_back({ objectId, pass.viewProjectionMatrix * object.modelMatrix });
    }
    for (int_t i = 0; i < renderpassCount; i++) {
        addObjectToRenderpass(i, objectId);
    }
}

void Scene::addDepthRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix) {
    DepthRenderpass pass;
    pass.framebuffer = framebuffer;
    pass.viewProjectionMatrix = viewProjectionMatrix;
    pass.entries.reserve(objects.size());
    for (id_t i = 0; i < objects.size(); i++)
        pass.entries.push_back({ i, pass.viewProjectionMatrix * objects[i].object.modelMatrix });
    depthPasses.push_back(std::move(pass));
}

int_t Scene::addRenderpass(Framebuffer* framebuffer, const Mat4x4& viewProjectionMatrix) {
    int_t index = renderpassCount++;
    Renderpass& renderpass = renderpasses[index];
    renderpass.framebuffer = framebuffer;
    renderpass.viewProjectionMatrix = viewProjectionMatrix;
    return index;
}

void Scene::addObjectToRenderpass(int_t renderpassIndex, id_t objectId) {
    Renderpass& renderpass = renderpasses[renderpassIndex];

    std::vector<EffectInstance*> effects;
    effects.reserve(globalEffects.size() + objects[objectId].effects.size());
    effects.insert(effects.end(), globalEffects.begin(), globalEffects.end());
    effects.insert(effects.end(), objects[objectId].effects.begin(), objects[objectId].effects.end());
    std::sort(effects.begin(), effects.end(), [](EffectInstance* first, EffectInstance* second) {
        return first->effect < second->effect;
    });

    Resource<id_t> handle = EffectShaderFactory::getHandle(effects);
    if (*handle == shaderEntries.size()) {
        shaderEntries.resize(*handle + 1);
        for (int_t i = 0; i < MAX_RENDERPASSES; i++) {
            shaderEntries[*handle].indices[i] = -1;
        }
    }

    ShaderEntry& shaderEntry = shaderEntries[*handle];
    Shaderpass* shaderpass;
    int_t shaderpassIndex = shaderEntry.indices[renderpassIndex];
    if (shaderpassIndex == -1) {
        shaderpassIndex = renderpass.passes.size();
        shaderEntry.indices[renderpassIndex] = shaderpassIndex;
        shaderpass = &renderpass.passes.emplace_back(handle);
    } else {
        shaderpass = &renderpass.passes[shaderpassIndex];
    }

    shaderpass->addEntry(objectId, effects);
}

void Scene::draw() {
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
            depthShader->uniformMat4x4(depthShader->mvpUniform, entry.modelViewProjectionMatrix);
            depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
            pass.framebuffer->draw(object.drawMode, object.indices);
        }
    }

    for (int_t i = renderpassCount - 1; i >= 0; i--) {
        Renderpass& renderpass = renderpasses[i];
        renderpass.framebuffer->bind();
        renderpass.framebuffer->clear();
        int_t rendererObjects = 0;
        for (Shaderpass& shaderpass : renderpass.passes) {
            shaderpass.draw(*this, renderpass);
        }
    }
}

#include "3d/Renderpass.hpp"
#include <algorithm>

void DepthRenderpass::draw(GraphicsObjectCollection& objects) {
    depthShader->use();
    framebuffer->bind();
    framebuffer->clear();
    for (id_t id : objectIds) {
        GraphicsObject& object = objects[id];
        depthShader->uniformMat4x4(depthShader->mvpUniform, viewProjectionMatrix * object.modelMatrix);
        depthShader->attribPointer(depthShader->positionAttrib, object.vertices);
        framebuffer->draw(object.drawMode, object.indices);
    }
}

void DepthRenderpass::addObject(id_t objectId) {
    objectIds.push_back(objectId);
}

void Renderpass::draw(GraphicsObjectCollection& objects) {
    framebuffer->bind();
    framebuffer->clear();
    for (std::unique_ptr<Shaderpass>& pass : passes) {
        pass->draw(objects, *this);
    }
}

void Renderpass::addObject(id_t objectId, std::vector<EffectInstance*> effects) {
    std::sort(effects.begin(), effects.end(), [](EffectInstance* first, EffectInstance* second) {
        return first->effect.id < second->effect.id;
    });
    Resource<id_t> handle = EffectShaderFactory::getHandle(effects);
    int_t index;
    for (index = 0; index < passes.size(); index++) {
        if (*passes[index]->shaderHandle == *handle)
            break;
    }
    if (index == passes.size()) {
        passes.emplace_back(Shaderpass::construct(handle));
    }
    passes[index]->addEntry(objectId, effects);

    auto it = std::lower_bound(objectIds.begin(), objectIds.end(), objectId);
    auto it2 = objectEntries.begin() + (it - objectIds.begin());
    objectIds.insert(it, objectId);
    objectEntries.insert(it2, { std::move(effects), index });
}

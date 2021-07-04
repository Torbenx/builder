#pragma once

#include "3d/Shaderpass.hpp"

struct Renderpass {
    Framebuffer* framebuffer;
    Mat4x4 viewProjectionMatrix;
    std::vector<std::unique_ptr<Shaderpass>> passes;

    Renderpass(Framebuffer* framebuffer)
        : framebuffer(framebuffer) { }

    void draw(GraphicsObjectCollection& objects) {
        framebuffer->bind();
        framebuffer->clear();
        for (std::unique_ptr<Shaderpass>& pass : passes) {
            pass->draw(objects, *this);
        }
    }

    void addObject(id_t objectId, std::vector<EffectInstance*> effects) {
        std::sort(effects.begin(), effects.end(), [](EffectInstance* first, EffectInstance* second) {
            return first->effect < second->effect;
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

private:
    struct ObjectEntry {
        std::vector<EffectInstance*> effects;
        int_t index;
    };
    std::vector<int_t> shaderIndices;
    std::vector<id_t> objectIds;
    std::vector<ObjectEntry> objectEntries;
};

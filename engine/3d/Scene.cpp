#include "3d/Scene.hpp"
#include "math/solver/LinearProgramming.hpp"
#include "util/LogTimer.hpp"
#include <unordered_set>

Scene::~Scene() {
    for (auto pair : shaders) {
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
    std::sort(effects.begin(), effects.end(), [](EffectInstance* first, EffectInstance* second) {
        return first->effect < second->effect;
    });
    ptrdiff_t hash = 0;
    for (EffectInstance* instance : effects) {
        hash ^= ((ptrdiff_t)instance->effect) >> 3;
    }

    /*LOGD("hash: %llx", hash);
    for (EffectInstance* instance : effects) {
        LOGD("0x%p: 0x%p", instance, instance->effect);
    }
    LOGD("");*/

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

struct BasicZeroTester {
    bool operator()(float f) {
        return std::abs(f) < 0.0005f;
    }

    bool is_positive(float f) {
        return f > 0.0005f;
    }
};

void Scene::draw() {
    /*for (int i = 0; i < renderpassCount; i++) {
        Renderpass& renderpass = renderpasses[i];
        for (Shaderpass& shaderpass : renderpass.passes) {
            std::unordered_set<EffectInstance*> set;
            for (auto& entry : shaderpass.entries) {
                for (auto& e : entry.effects) {
                    set.insert(e.effect);
                }
            }
            LOGD("enties=%u, instances=%u, depth=%u", shaderpass.entries.size(), set.size(), shaderpass.entries[0].effects.size());
            if (set.size() == 6 && shaderpass.entries.size() == 3) {
                bool matrix[3 * 6];
                auto it = set.begin();
                for (int i = 0; i < 6; i++, it++) {
                    for (int j = 0; j < 3; j++) {
                        bool found = false;
                        for (auto& e : shaderpass.entries[j].effects) {
                            if (e.effect == *it) {
                                found = true;
                                break;
                            }
                        }
                        matrix[i * 3 + j] = found;
                    }
                    LOGD("%u %u %u", matrix[i * 3 + 0], matrix[i * 3 + 1], matrix[i * 3 + 2]);
                }
            }
        }
    }*/

    LinearProgramming<BasicZeroTester> lp;

    uint32_t k = 4;
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
    }

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
    {
        LogTimer timer;
        lp.doStuff();
    }
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
            shaderpass.shader->use();
            shaderpass.shader->uniformMat4x4(shaderpass.shader->viewProjectionUniform, renderpass.viewProjectionMatrix);
            for (Shaderpass::Entry& entry : shaderpass.entries) {
                Object& object = objects[entry.objectId].object;

                if (!isVisable(renderpass.viewProjectionMatrix * entry.modelMatrix, object))
                    continue;

                for (auto& effect : entry.effects) {
                    effect.effect->applyTo(*shaderpass.shader, effect.bindings);
                }
                shaderpass.shader->uniformMat4x4(shaderpass.shader->modelUniform, entry.modelMatrix);
                shaderpass.shader->attribPointer(shaderpass.shader->positionAttrib, object.vertices);
                shaderpass.shader->attribPointer(shaderpass.shader->normalAttrib, object.normals);
                renderpass.framebuffer->draw(object.drawMode, object.indices);
            }
        }
    }
}

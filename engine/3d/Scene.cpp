#include "3d/Scene.hpp"
#include "math/LUDecomposition.hpp"
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

    int width = 40;
    int height = 40;
    SparseMatrix<float, int, BasicZeroTester> matrix;
    matrix.setEmpty(width);
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            if ((i + j) % 10 == 0)
                matrix.columns[i].addTo(j, sin(i * j + 1));
        }
    }
    matrix.debug();
    LUPDecomposition<float, int, BasicZeroTester> lu;
    {
        LogTimer timer;
        lu.decompose(std::move(matrix));
    }

    /*LOGD("u: %u x %u", lu.u.columns.size(), lu.u.rowCount());
    lu.u.debug();
    LOGD("l: %u x %u", lu.l.columns.size(), lu.l.rowCount());
    lu.l.debug();*/

    SparseMatrix<float, int, BasicZeroTester>::Column col;
    for (int i = 0; i < height; i++) {
        col.addTo(i, i / (float)height);
    }
    col.addTo(0, 1.0f);
    {
        LogTimer timer;
        lu.replaceColumn(20, std::move(col));
    }

    /*LOGD("u: %u x %u", lu.u.columns.size(), lu.u.rowCount());
    lu.u.debug();
    LOGD("l: %u x %u", lu.l.columns.size(), lu.l.rowCount());
    lu.l.debug();*/

    {
        LogTimer timer;
        matrix = SparseMatrix<float, int, BasicZeroTester>::multiply(lu.l, lu.u);
        matrix.add(lu.u, 1);
    }
    SparseMatrix<float, int, BasicZeroTester> out;
    out.columns.resize(width);
    for (int i = 0; i < width; i++) {
        std::swap(out.columns[i], matrix.columns[lu.p[i]]);
    }
    out.debug();

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
        return fabs(volume) > 0.0001f || std::isnan(volume);
    };

    uint64_t start = getNanos();
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
    uint64_t end = getNanos();
    // LOGD("draw took %f us", (end - start) / 1000.0);
}

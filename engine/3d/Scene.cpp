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

void Scene::addObject(Object* obj, Mat4x4& model) {
    ColorSource colorSource = obj->getColorSource();
    bool hasReflection = obj->hasReflection();
    ObjectData objData;
    objData.object = obj;
    objData.model = model;
    for (ShaderInstance& shader : shaders) {
        Object* otherObj = shader.objects.front().object;
        if (colorSource == otherObj->getColorSource() && hasReflection == otherObj->hasReflection()) {
            shader.objects.push_back(objData);
            return;
        }
    }
    shaders.resize(shaders.size() + 1);
    shaders.back().objects.push_back(objData);
}

void Scene::draw(Mat4x4 vp) {
    static auto thiz = this;
    void (*doDepthDraw)(Framebuffer & framebuffer, Shader & shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform)
        = [](Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform) {
              for (ShaderInstance& inst : thiz->shaders) {
                  for (ObjectData& objData : inst.objects) {
                      shader.uniformMat4x4(modelUniform, objData.model);
                      objData.object->draw(framebuffer, shader, positionAttrib);
                  }
              }
          };
    for (PointLight* light : pointLights) {
        light->draw(doDepthDraw);
    }
    for (SpotLight* light : spotLights) {
        light->draw(doDepthDraw);
    }
    for (auto* ref : reflections) {
        ref->framebuffer.bind();
        ref->framebuffer.clear();
        Mat4x4 rvp = vp * ref->getReflectionMatrix();
        doDraw(ref->framebuffer, rvp, false);
    }
    Framebuffer::DEFAULT.bind();
    Framebuffer::DEFAULT.clear();
    doDraw(Framebuffer::DEFAULT, vp);
}

void Scene::doDraw(Framebuffer& framebuffer, Mat4x4& vp, bool drawReflections) {
    for (ShaderInstance& shader : shaders) {
        Object* obj = shader.objects.front().object;
        Reflection* ref = obj->getReflection();
        if (ref && !drawReflections)
            continue;
        if (!shader.shader) {
            LShaderBuilder builder;
            shader.data.positionAttrib = builder.addAttrib("vec3", "a_position");
            builder.rawPosition = "a_position";
            shader.data.normalAttrib = builder.addAttrib("vec3", "a_normal");
            builder.rawNormal = "a_normal";
            shader.modelUniform = builder.addVertexUniform("mat4", "u_model");
            builder.rawToWorldMatrix = "u_model";
            shader.vpUniform = builder.addVertexUniform("mat4", "u_mvpMatrix");
            builder.worldToScreenMatrix = "u_mvpMatrix";
            switch (obj->getColorSource()) {
            case Uniform:
                shader.data.colorSource.uniform = builder.addFragmentUniform("vec3", "u_color");
                builder.fragmentDiffuseColor = "u_color";
                break;
            case Texture:
                shader.data.colorSource.texture.uniform = builder.addFragmentUniform("sampler2D", "u_texture");
                shader.data.colorSource.texture.positionAttrib = builder.addAttrib("vec2", "a_texCoord");
                builder.addVarying("vec2", "v_texCoord");
                builder.vMain << "v_texCoord = a_texCoord;\n";
                builder.fMain << "vec3 texColor = texture(u_texture, v_texCoord).rgb;\n";
                builder.fragmentDiffuseColor = "texColor";
                break;
            default:
                LOGW("INVALID COLOR SOURCE!!!");
            }
            for (AmbientLight* l : ambientLights) {
                l->addTo(builder);
            }
            for (SpotLight* l : spotLights) {
                l->addTo(builder);
            }
            for (PointLight* l : pointLights) {
                l->addTo(builder);
            }
            if (ref) {
                ref->addTo(builder);
            }
            shader.shader.reset(new LShader(builder));
        }
        shader.shader->use();
        int effectHandle = 0;
        for (AmbientLight* l : ambientLights) {
            shader.shader->bindEffect({ effectHandle++ }, *l);
        }
        for (SpotLight* l : spotLights) {
            shader.shader->bindEffect({ effectHandle++ }, *l);
        }
        for (PointLight* l : pointLights) {
            shader.shader->bindEffect({ effectHandle++ }, *l);
        }
        if (ref) {
            shader.shader->bindEffect({ effectHandle++ }, *ref);
        }
        shader.shader->uniformMat4x4(shader.vpUniform, vp);
        for (ObjectData& objData : shader.objects) {
            shader.shader->uniformMat4x4(shader.modelUniform, objData.model);
            objData.object->draw2(framebuffer, *shader.shader, shader.data);
        }
    }
}

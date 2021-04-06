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

void Scene::addObject(Object* obj) {
    ColorSource colorSource = obj->getColorSource();
    bool hasReflection = obj->hasReflection();
    renderables.push_back(obj);
    for (ShaderInstance& shader : shaders) {
        Object* otherObj = shader.objects.front();
        if (colorSource == otherObj->getColorSource() && hasReflection == otherObj->hasReflection()) {
            shader.objects.push_back(obj);
            return;
        }
    }
    shaders.resize(shaders.size() + 1);
    shaders.back().objects.push_back(obj);
}

void Scene::draw(Mat4x4 vp) {
    for (PointLight* light : pointLights) {
        light->draw(renderables.data(), renderables.size());
    }
    for (SpotLight* light : spotLights) {
        light->draw(renderables.data(), renderables.size());
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
        Object* obj = shader.objects.front();
        if (obj->hasReflection() && !drawReflections)
            continue;
        if (!shader.shader) {
            LShaderBuilder builder;
            shader.data.positionAttrib = builder.addAttrib("vec3", "a_position");
            builder.rawPosition = "a_position";
            shader.data.normalAttrib = builder.addAttrib("vec3", "a_normal");
            builder.rawNormal = "a_normal";
            shader.data.modelUniform = builder.addVertexUniform("mat4", "u_model");
            builder.rawToWorldMatrix = "u_model";
            shader.mvpMatrixHandle = builder.addVertexUniform("mat4", "u_mvpMatrix");
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
            Reflection* ref = obj->getReflection();
            if (ref) {
                ref->addTo(builder);
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
            shader.shader.reset(new LShader(builder));
        }
        shader.shader->use();
        shader.shader->applyEffects();
        shader.shader->uniformMat4x4(shader.mvpMatrixHandle, vp);
        for (Object* obj : shader.objects) {
            obj->draw2(framebuffer, *shader.shader, shader.data);
        }
    }
}

#include "3d/Scene.hpp"

Scene::Scene(){

}

Scene::~Scene(){

}

void Scene::addLight(PointLight& light){
    pointLights.push_back(&light);
}

void Scene::addLight(SpotLight& light){
    spotLights.push_back(&light);
}

void Scene::addLight(AmbientLight& light){
    ambientLights.push_back(&light);
}

void Scene::addReflection(Reflection& ref){
    reflections.push_back(&ref);
}

bool compareTextureTypes(std::string *first, std::string *second, int n){
    if(n==0)return true;
    for(int i=0;i<n;i++){
        if(first[i] != second[i])return false;
    }
    return true;
}

bool compareReflections(Reflection **first, Reflection **second, int n){
    if(n==0)return true;
    for(int i=0;i<n;i++){
        bool found = false;
        for(int j=0;j<n;j++){
            if(first[i] == second[j]){
                found = true;
                break;
            }
        }
        if(!found)return false;
    }
    return true;
}

void Scene::addObject(SceneObject *obj){
    ColorSources colors = obj->getColorSources();
    renderables.push_back(obj);
    for(std::list<SceneShader>::iterator it = shaders.begin(); it != shaders.end(); it++){
        SceneObject *otherObj = it->objects.front();
        ColorSources otherColors = otherObj->getColorSources();
        if(
            otherColors.nUniforms == colors.nUniforms &&
            otherColors.nTextures == colors.nTextures &&
            otherColors.nAttribs == colors.nAttribs &&
            otherColors.nReflections == colors.nReflections &&
            compareTextureTypes(otherObj->getTextureTypes(), obj->getTextureTypes(), colors.nTextures) &&
            compareReflections(otherObj->getReflections(), obj->getReflections(), colors.nReflections)
        ){
            it->objects.push_back(obj);
        }
    }
    shaders.resize(shaders.size()+1);
    shaders.back().objects.push_back(obj);
}

void Scene::draw(Mat4x4 vp){
    for(std::vector<PointLight*>::iterator lit=pointLights.begin();lit!=pointLights.end();lit++){
        (*lit)->draw(renderables.data(), renderables.size());
    }
    for(std::vector<SpotLight*>::iterator lit=spotLights.begin();lit!=spotLights.end();lit++){
        (*lit)->draw(renderables.data(), renderables.size());
    }
    for(std::vector<Reflection*>::iterator rit=reflections.begin();rit!=reflections.end();rit++){
        Framebuffer& framebuffer = (*rit)->framebuffer;
        framebuffer.bind();
        framebuffer.clear();
        Mat4x4 rvp = vp * (*rit)->getReflectionMatrix();
        doDraw(framebuffer, rvp, false);
    }
    Framebuffer::DEFAULT.bind();
    Framebuffer::DEFAULT.clear();
    doDraw(Framebuffer::DEFAULT, vp);
}

void Scene::doDraw(Framebuffer& framebuffer, Mat4x4& vp, bool drawReflections){
    for(SceneShader& shader : shaders){
        SceneObject *obj = shader.objects.front();
        ColorSources colors = obj->getColorSources();
        if(colors.nReflections > 0 && !drawReflections)continue;
        if(!shader.shader){
            LShaderBuilder builder;
            shader.data.positionAttrib = builder.addAttrib("vec3", "a_position");
            builder.rawPosition = "a_position";
            shader.data.normalAttrib = builder.addAttrib("vec3", "a_normal");
            builder.rawNormal = "a_normal";
            shader.data.modelUniform = builder.addVertexUniform("mat4", "u_model");
            builder.rawToWorldMatrix = "u_model";
            shader.mvpMatrixHandle = builder.addVertexUniform("mat4", "u_mvpMatrix");
            builder.worldToScreenMatrix = "u_mvpMatrix";
            shader.data.uniforms = new Shader::LocationHandle[colors.nUniforms];
            for(int i=0;i<colors.nUniforms;i++){
                shader.data.uniforms[i] = builder.addFragmentUniform("vec3", "u_color");
                builder.fragmentDiffuseColor = "u_color";
            }
            shader.data.textureTypes = obj->getTextureTypes();
            shader.data.textureUniforms = new Shader::LocationHandle[colors.nTextures];
            shader.data.texPositionAttribs = new Shader::LocationHandle[colors.nTextures];
            for(int i=0;i<colors.nTextures;i++){
                shader.data.textureUniforms[i] = builder.addFragmentUniform("sampler2D", "u_texture");
                shader.data.texPositionAttribs[i] = builder.addAttrib("vec2", "a_texCoord");
                builder.addVarying("vec2", "v_texCoord");
                builder.vMain << "v_texCoord = a_texCoord;\n";
                builder.fMain << "vec3 texColor = texture(u_texture, v_texCoord).rgb;\n";
                builder.fragmentDiffuseColor = "texColor";
            }
            shader.data.attribs = new Shader::LocationHandle[colors.nAttribs];
            for(int i=0;i<colors.nAttribs;i++){

            }
            Reflection **refs = obj->getReflections();
            for(int i=0;i<colors.nReflections;i++){
                refs[i]->addTo(builder);
            }
            for(AmbientLight* l : ambientLights){
                l->addTo(builder);
            }
            for(SpotLight* l : spotLights){
                l->addTo(builder);
            }
            for(PointLight* l : pointLights){
                l->addTo(builder);
            }
            shader.shader.reset(new LShader(builder));
        }
        shader.shader->use();
        shader.shader->applyEffects();
        shader.shader->uniformMat4x4(shader.mvpMatrixHandle, vp);
        for(SceneObject* obj : shader.objects){
            obj->draw2(framebuffer, *shader.shader, shader.data);
        }
    }
}

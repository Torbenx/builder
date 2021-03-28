#include "Scene.hpp"

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

void Scene::draw(MvpMatrix mvp){
    for(std::vector<PointLight*>::iterator lit=pointLights.begin();lit!=pointLights.end();lit++){
        (*lit)->draw(renderables.data(), renderables.size());
    }
    for(std::vector<SpotLight*>::iterator lit=spotLights.begin();lit!=spotLights.end();lit++){
        (*lit)->draw(renderables.data(), renderables.size());
    }
    for(std::vector<Reflection*>::iterator rit=reflections.begin();rit!=reflections.end();rit++){
        (*rit)->bind();
        (*rit)->getReflectionModel(mvp.model);
        doDraw(mvp, false);
        Framebuffer::DEFAULT.bind();
    }
    mvp.model = Mat4x4::Id;
    doDraw(mvp);
}

void Scene::doDraw(MvpMatrix& mvp, bool drawReflections){
    Mat4x4 matrix = mvp.setupMvp();
    for(auto it = shaders.begin(); it != shaders.end(); it++){
        SceneObject *obj = it->objects.front();
        ColorSources colors = obj->getColorSources();
        if(colors.nReflections > 0 && !drawReflections)continue;
        if(it->needsInit()){
            it->data.positionAttrib = it->addAttrib("vec3", "a_position");
            it->rawPosition = "a_position";
            it->data.normalAttrib = it->addAttrib("vec3", "a_normal");
            it->rawNormal = "a_normal";
            it->data.modelUniform = it->addVertexUniform("mat4", "u_model");
            it->rawToWorldMatrix = "u_model";
            it->mvpMatrixHandle = it->addVertexUniform("mat4", "u_mvpMatrix");
            it->worldToScreenMatrix = "u_mvpMatrix";
            it->data.uniforms = new Shader::LocationHandle[colors.nUniforms];
            for(int i=0;i<colors.nUniforms;i++){
                it->data.uniforms[i] = it->addFragmentUniform("vec3", "u_color");
                it->fragmentDiffuseColor = "u_color";
            }
            it->data.textureTypes = obj->getTextureTypes();
            it->data.textureUniforms = new Shader::LocationHandle[colors.nTextures];
            it->data.texPositionAttribs = new Shader::LocationHandle[colors.nTextures];
            for(int i=0;i<colors.nTextures;i++){
                it->data.textureUniforms[i] = it->addFragmentUniform("sampler2D", "u_texture");
                it->data.texPositionAttribs[i] = it->addAttrib("vec2", "a_texCoord");
                it->addVarying("vec2", "v_texCoord");
                it->vMain << "v_texCoord = a_texCoord;\n";
                it->fMain << "vec3 texColor = texture2D(u_texture, v_texCoord).rgb;\n";
                it->fragmentDiffuseColor = "texColor";
            }
            it->data.attribs = new Shader::LocationHandle[colors.nAttribs];
            for(int i=0;i<colors.nAttribs;i++){

            }
            Reflection **refs = obj->getReflections();
            for(int i=0;i<colors.nReflections;i++){
                refs[i]->addTo(*it);
            }
            for(AmbientLight* l : ambientLights){
                l->addTo(*it);
            }
            for(SpotLight* l : spotLights){
                l->addTo(*it);
            }
            for(PointLight* l : pointLights){
                l->addTo(*it);
            }
            it->init();
        }
        it->use();
        it->applyEffects();
        it->uniformMat4x4(it->mvpMatrixHandle, matrix);
        for(SceneObject* obj : it->objects){
            obj->draw2(*it, it->data);
        }
    }
}

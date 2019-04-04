#include "Scene.hpp"

Scene::Scene(){
    baseBuilder.setVertexPositionAttrib("a_position");
    baseBuilder.setScreenMvpMatrixUniform("u_mvpMatrix");
    baseBuilder.setWorldModelMatrixUniform("u_model");
}

Scene::~Scene(){
    for(std::list<SceneShader>::iterator it = shaders.begin(); it != shaders.end(); it++){
        delete it->shader;
    }
}

void Scene::addLight(PointLight& light){
    baseBuilder.addLight(light);
    pointLights.push_back(&light);
}

void Scene::addLight(SpotLight& light){
    baseBuilder.addLight(light);
    spotLights.push_back(&light);
}

void Scene::addLight(AmbientLight& light){
    baseBuilder.addLight(light);
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
    SceneShader shader;
    shader.shader = nullptr;
    shader.objects.push_back(obj);
    shaders.push_back(shader);
}

void Scene::draw(MvpMatrix mvp){
    for(std::vector<PointLight*>::iterator lit=pointLights.begin();lit!=pointLights.end();lit++){
        (*lit)->draw(Mat4x4::Id, renderables.data(), renderables.size());
    }
    for(std::vector<SpotLight*>::iterator lit=spotLights.begin();lit!=spotLights.end();lit++){
        (*lit)->draw(Mat4x4::Id, renderables.data(), renderables.size());
    }
    for(std::vector<Reflection*>::iterator rit=reflections.begin();rit!=reflections.end();rit++){
        (*rit)->bind();
        (*rit)->getReflectionModel(mvp.model);
        doDraw(mvp, false);
        (*rit)->unbind();
    }
    mvp.model = Mat4x4::Id;
    doDraw(mvp);
}

void Scene::doDraw(MvpMatrix& mvp, bool drawReflections){
    Mat4x4 matrix = mvp.setupMvp();
    for(std::list<SceneShader>::iterator it = shaders.begin(); it != shaders.end(); it++){
        LShader *shader = it->shader;
        SceneObject *obj = it->objects.front();
        ColorSources colors = obj->getColorSources();
        if(colors.nReflections > 0 && !drawReflections)continue;
        if(!shader){
            ShaderBuilder builder(baseBuilder);
            it->data.positionAttrib = {"a_position"};
            it->data.modelUniform = {"u_model"};
            it->data.uniforms = new ShaderLocationHandle[colors.nUniforms];
            for(int i=0;i<colors.nUniforms;i++){
                std::string s = builder.newUniform();
                it->data.uniforms[i] = {s};
                builder.setFragmentColorUniform(s);
            }
            it->data.textureTypes = obj->getTextureTypes();
            it->data.textureUniforms = new ShaderLocationHandle[colors.nTextures];
            it->data.texPositionAttribs = new ShaderLocationHandle[colors.nTextures];
            for(int i=0;i<colors.nTextures;i++){
                std::string u = builder.newUniform();
                std::string a = builder.newAttrib();
                it->data.textureUniforms[i] = {u};
                it->data.texPositionAttribs[i] = {a};
                builder.setFragmentColorUniformTexture(it->data.textureTypes[i], u, a);
            }
            it->data.attribs = new ShaderLocationHandle[colors.nAttribs];
            for(int i=0;i<colors.nAttribs;i++){
                std::string s = builder.newAttrib();
                it->data.attribs[i] = {s};
                builder.setFragmentColorAttrib(s);
            }
            Reflection **refs = obj->getReflections();
            for(int i=0;i<colors.nReflections;i++){
                builder.addReflection(*refs[i]);
            }
            shader = builder.build();
            it->shader = shader;
            shader->init();
        }
        shader->use();
        Reflection **refs = obj->getReflections();
        for(int i=0;i<colors.nReflections;i++){
            shader->reflection(*refs[i]);
        }
        for(std::vector<PointLight*>::iterator lit=pointLights.begin();lit!=pointLights.end();lit++){
            shader->light(**lit, mvp.model);
        }
        for(std::vector<SpotLight*>::iterator lit=spotLights.begin();lit!=spotLights.end();lit++){
            shader->light(**lit, mvp.model);
        }
        for(std::vector<AmbientLight*>::iterator lit=ambientLights.begin();lit!=ambientLights.end();lit++){
            shader->light(**lit);
        }
        shader->uniformMat4x4("u_mvpMatrix", matrix);
        for(std::vector<SceneObject*>::iterator oit=it->objects.begin();oit!=it->objects.end();oit++){
            (*oit)->draw2(shader, it->data);
        }
    }
}
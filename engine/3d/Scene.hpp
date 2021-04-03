#pragma once

#include "lgraphic/LShader.hpp"
#include "lgraphic/AmbientLight.hpp"
#include "lgraphic/SpotLight.hpp"
#include "lgraphic/PointLight.hpp"
#include "lgraphic/Reflection.hpp"
#include <vector>
#include <list>
#include <memory>

struct ColorSources{
    int nUniforms = 0;
    int nTextures = 0;
    int nAttribs = 0;
    int nReflections = 0;
};

struct SceneDrawData{
    Shader::LocationHandle positionAttrib;
    Shader::LocationHandle normalAttrib;
    Shader::LocationHandle modelUniform;
    Shader::LocationHandle *uniforms;
    std::string *textureTypes;
    Shader::LocationHandle *textureUniforms;
    Shader::LocationHandle *texPositionAttribs;
    Shader::LocationHandle *attribs;
};

class SceneShader;

class SceneObject : public Renderable{
public:
    virtual ColorSources getColorSources() = 0;
    virtual std::string *getTextureTypes(){ return nullptr; }
    virtual Reflection **getReflections(){ return nullptr; }
    virtual void draw2(Framebuffer& framebuffer, LShader& shader, const SceneDrawData& data) = 0;
};

class SceneShader{
public:
    std::unique_ptr<LShader> shader;
    SceneDrawData data;
    Shader::LocationHandle mvpMatrixHandle;
    std::vector<SceneObject*> objects;
};

class Scene{
public:
    Scene();
    ~Scene();
    void addLight(PointLight& light);
    void addLight(SpotLight& light);
    void addLight(AmbientLight& light);
    void addReflection(Reflection& ref);
    void addObject(SceneObject *obj);
    void draw(Mat4x4 mvp);
private:
    std::list<SceneShader> shaders;
    std::vector<Renderable*> renderables;
    std::vector<PointLight*> pointLights;
    std::vector<SpotLight*> spotLights;
    std::vector<AmbientLight*> ambientLights;
    std::vector<Reflection*> reflections;
    void doDraw(Framebuffer& framebuffer, Mat4x4& mvp, bool drawReflections=true);
};

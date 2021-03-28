#ifndef HEADER_3D_SCENE_H
#define HEADER_3D_SCENE_H

#include "../lgraphic/LShader.hpp"
#include "../lgraphic/light/AmbientLight.hpp"
#include "../lgraphic/light/SpotLight.hpp"
#include "../lgraphic/light/PointLight.hpp"
#include "../lgraphic/reflection/Reflection.hpp"
#include <vector>
#include <list>

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
        virtual void draw2(SceneShader& shader, const SceneDrawData& data) = 0;
};

class SceneShader : public LShader{
public:
    SceneDrawData data;
    Shader::LocationHandle mvpMatrixHandle;
    std::vector<SceneObject*> objects;
};

class Scene{
    public:
        std::list<SceneShader> shaders;
        std::vector<Renderable*> renderables;
        std::vector<PointLight*> pointLights;
        std::vector<SpotLight*> spotLights;
        std::vector<AmbientLight*> ambientLights;
        std::vector<Reflection*> reflections;
        Scene();
        ~Scene();
        void addLight(PointLight& light);
        void addLight(SpotLight& light);
        void addLight(AmbientLight& light);
        void addReflection(Reflection& ref);
        void addObject(SceneObject *obj);
        void draw(MvpMatrix mvp);
        void doDraw(MvpMatrix& mvp, bool drawReflections=true);
};



#endif

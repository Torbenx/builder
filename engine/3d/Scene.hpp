#ifndef HEADER_3D_SCENE_H
#define HEADER_3D_SCENE_H

#include "../lgraphic/ShaderBuilder.hpp"
#include <vector>

struct ColorSources{
    int nUniforms = 0;
    int nTextures = 0;
    int nAttribs = 0;
    int nReflections = 0;
};

struct ShaderLocationHandle{
    std::string name;
};

struct SceneDrawData{
    ShaderLocationHandle positionAttrib;
    ShaderLocationHandle normalAttrib;
    ShaderLocationHandle modelUniform;
    ShaderLocationHandle *uniforms;
    std::string *textureTypes;
    ShaderLocationHandle *textureUniforms;
    ShaderLocationHandle *texPositionAttribs;
    ShaderLocationHandle *attribs;
};

class SceneObject : public Renderable{
    public:
        virtual ColorSources getColorSources() = 0;
        virtual std::string *getTextureTypes(){ return nullptr; }
        virtual Reflection **getReflections(){ return nullptr; }
        virtual void draw2(LShader *shader, SceneDrawData& data) = 0;
};

struct SceneShader{
    LShader* shader;
    SceneDrawData data;
    std::vector<SceneObject*> objects;
};

class Scene{
    public:
        ShaderBuilder baseBuilder;
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

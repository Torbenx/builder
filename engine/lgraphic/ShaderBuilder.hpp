#ifndef HEADER_LGRAPHIC_SHADERBUILDER_H
#define HEADER_LGRAPHIC_SHADERBUILDER_H

#include "../graphic/Shader.hpp"
#include <list>
#include "../util/log.hpp"
#include "LShader.hpp"
#include <sstream>

class ShaderBuilder{
    public:
        std::string rawPosition,rawNormal,worldPosition = "v_worldPos",worldNormal = "v_normal",screenPosition = "gl_Position",fColor = "color",lightIntensity = "lightIntensity",fColorDiffuse = "colorDiffuse",worldPositionMulti,rawPositionMulti;
        std::map<void*,std::string> locations;
        int uCount = 0,aCount = 0,vCount = 0,varCount = 0;
        std::ostringstream vMain,fMain,vInit,fInit;
        ShaderBuilder();
        ShaderBuilder(const ShaderBuilder& source);
        void addLight(AmbientLight& l);
        void addLight(SpotLight& l);
        void addLight(PointLight& l);
        void addReflection(Reflection& r);
        void addToFragmentColor(std::string s);
        void setFragmentColorUniform(std::string s);
        void setFragmentColorUniformTexture(std::string texType, std::string textureUniform, std::string texCoordAttrib);
        void setFragmentColorAttrib(std::string s);
        void setScreenMvpMatrixUniform(std::string s);
        void setWorldModelMatrixUniform(std::string s);
        void setVertexPositionAttrib(std::string s);
        void setVertexNormalAttrib(std::string s);
        LShader* build();
        std::string buildVStr();
        std::string buildFStr();
        std::string newUniform(void *obj);
        std::string newUniform();
        std::string newAttrib(void *obj);
        std::string newAttrib();
        std::string newVarying();
        std::string newVar();
        void addVarying(std::string type,std::string name);
        void addVertexUniform(std::string type,std::string name);
        void addFragmentUniform(std::string type,std::string name);
        void addAttrib(std::string type,std::string name);
};

#endif

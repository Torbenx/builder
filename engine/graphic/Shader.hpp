#ifndef HEADER_GRAPHIC_SHADER_H
#define HEADER_GRAPHIC_SHADER_H

#include "../gl/glwrapper.hpp"
#include "../math/Vec4.hpp"
#include "Buffer.hpp"
#include "Texture.hpp"
#include "CubeMap.hpp"
#include <string>
#include <vector>
#include <sstream>

class ShaderBuilder;

class Shader{
public:
    static const GLenum slots[];

    struct LocationHandle{
        GLuint handle;
    };

    struct Location{
        std::string name;
        bool isUniform;
        Location(std::string name, bool isUniform){
            this->name = name;
            this->isUniform = isUniform;
        }
    };

    std::vector<GLuint> locations;

    Shader(ShaderBuilder& builder){ initalize(builder); }
    virtual ~Shader();

    void initalize(ShaderBuilder& builder);
    void use();

    GLint getUniform(LocationHandle handle);
    GLuint getAttrib(LocationHandle handle);
    GLuint getAndEnableAttrib(LocationHandle handle);

    void uniform1f(LocationHandle uniform,float u);
    void uniform1i(LocationHandle uniform,int u);
    void uniform2f(LocationHandle uniform,float u1,float u2);
    void uniform2f(LocationHandle uniform,float *u);
    void uniform2i(LocationHandle uniform,int u1,int u2);
    void uniform2i(LocationHandle uniform,int *u);
    void uniform3f(LocationHandle uniform,float u1,float u2,float u3);
    void uniform3f(LocationHandle uniform,float *u);
    void uniform3i(LocationHandle uniform,int u1,int u2,int u3);
    void uniform3i(LocationHandle uniform,int *u);
    void uniform4f(LocationHandle uniform,float u1,float u2,float u3,float u4);
    void uniform4f(LocationHandle uniform,float *u);
    void uniform4i(LocationHandle uniform,int u1,int u2,int u3,int u4);
    void uniform4i(LocationHandle uniform,int *u);
    void uniformMatrix2f(LocationHandle uniform,float *u);
    void uniformMatrix3f(LocationHandle uniform,float *u);
    void uniformMatrix4f(LocationHandle uniform,float *u);
    void uniformVec2(LocationHandle uniform,Vec2 u);
    void uniformVec3(LocationHandle uniform,Vec3 u);
    void uniformVec4(LocationHandle uniform,Vec4 u);
    void uniformMat4x4(LocationHandle uniform,Mat4x4 u);
    void uniformTexture(LocationHandle uniform,Texture& u,GLuint slot);
    void uniformCubeMap(LocationHandle uniform,CubeMap& u,GLuint slot);

    void attribPointer(LocationHandle attrib,int size,float *a);
    void attribPointer(LocationHandle attrib,int size,int stride,float *a);
    void attribPointer(LocationHandle attrib,int size,Bufferf& a);
    void attribPointer(LocationHandle attrib,int offset,int size,int stride,Bufferf& a);

protected:
    Shader(){}

private:
    Shader(const Shader& other);
    GLuint glId = -1;
};

class ShaderBuilder{
public:
    std::ostringstream vInit, vMain, fInit, fMain;

    virtual ~ShaderBuilder(){}
    void newContext();
    std::string uniqueName(std::string name);
    Shader::LocationHandle addAttrib(std::string type, std::string name);
    Shader::LocationHandle addUniform(std::string type, std::string name, bool vertex);
    Shader::LocationHandle addVertexUniform(std::string type, std::string name){ return addUniform(type, name, true); }
    Shader::LocationHandle addFragmentUniform(std::string type, std::string name){ return addUniform(type, name, false); }
    void addVarying(std::string type, std::string name);
    int nextTextureSlot();

protected:
    virtual std::string buildVertexSource();
    virtual std::string buildFragmentSource();

private:
    std::vector<Shader::Location> locations;
    int contextCount = 0;
    int textureSlot = 0;

    friend Shader;
};

#endif

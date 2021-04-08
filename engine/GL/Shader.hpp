#pragma once

#include "GL/Buffer.hpp"
#include "GL/CubeMap.hpp"
#include "GL/Texture.hpp"
#include "GL/glwrapper.hpp"
#include "math/constants.hpp"
#include "util/target.hpp"
#include <sstream>
#include <string>
#include <vector>

class ShaderBuilder;

class Shader {
public:
    struct LocationHandle {
        GLuint handle;
    };

    struct Location {
        std::string name;
        bool isUniform;
        Location(std::string name, bool isUniform) {
            this->name = name;
            this->isUniform = isUniform;
        }
    };

    static const std::string FRAGMENT_COLOR;

    std::vector<GLuint> locations;

    Shader(ShaderBuilder& builder) { initalize(builder); }
    Shader(Shader&&);
    virtual ~Shader();

    void initalize(ShaderBuilder& builder);
    void use();

    GLint getUniform(LocationHandle handle);
    GLuint getAttrib(LocationHandle handle);
    GLuint getAndEnableAttrib(LocationHandle handle);

    void uniform1f(LocationHandle uniform, float u);
    void uniform1i(LocationHandle uniform, int u);
    void uniform2f(LocationHandle uniform, float u1, float u2);
    void uniform2f(LocationHandle uniform, float* u);
    void uniform2i(LocationHandle uniform, int u1, int u2);
    void uniform2i(LocationHandle uniform, int* u);
    void uniform3f(LocationHandle uniform, float u1, float u2, float u3);
    void uniform3f(LocationHandle uniform, float* u);
    void uniform3i(LocationHandle uniform, int u1, int u2, int u3);
    void uniform3i(LocationHandle uniform, int* u);
    void uniform4f(LocationHandle uniform, float u1, float u2, float u3, float u4);
    void uniform4f(LocationHandle uniform, float* u);
    void uniform4i(LocationHandle uniform, int u1, int u2, int u3, int u4);
    void uniform4i(LocationHandle uniform, int* u);
    void uniformMatrix2f(LocationHandle uniform, float* u);
    void uniformMatrix3f(LocationHandle uniform, float* u);
    void uniformMatrix4f(LocationHandle uniform, float* u);
    void uniformVec2(LocationHandle uniform, Vec2 u);
    void uniformVec3(LocationHandle uniform, Vec3 u);
    void uniformVec4(LocationHandle uniform, Vec4 u);
    void uniformMat4x4(LocationHandle uniform, Mat4x4& u);
    void uniformTexture(LocationHandle uniform, Texture& u, GLuint slot);
    void uniformCubeMap(LocationHandle uniform, CubeMap& u, GLuint slot);

    void attribPointer(LocationHandle attrib, int size, void* a);
    void attribPointer(LocationHandle attrib, int size, int stride, void* a);
    void attribPointer(LocationHandle attrib, VertexBuffer& a);
    void attribPointer(LocationHandle attrib, int stride, Buffer<float>& a);

    template<typename T>
    void attribPointer(LocationHandle attrib, T* a) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribPointer(getAndEnableAttrib(attrib), VectorTraits<T>::length, GL_FLOAT, GL_FALSE, 0, a);
    }

protected:
    Shader() { }

private:
    Shader(const Shader& other);
    GLuint glId = -1;
};

template void Shader::attribPointer<float>(LocationHandle, float*);
template void Shader::attribPointer<Vec2>(LocationHandle, Vec2*);
template void Shader::attribPointer<Vec3>(LocationHandle, Vec3*);
template void Shader::attribPointer<Vec4>(LocationHandle, Vec4*);

class ShaderBuilder {
public:
    std::ostringstream vExt, vInit, vMain, fExt, fInit, fMain;

    ShaderBuilder();
    virtual ~ShaderBuilder() { }
    void newContext();
    std::string uniqueName(std::string name);
    Shader::LocationHandle addAttrib(std::string type, std::string name, std::string qualifiers = "");
    Shader::LocationHandle addUniform(std::string type, std::string name, std::string qualifiers, bool vertex);
    Shader::LocationHandle addVertexUniform(std::string type, std::string name, std::string qualifiers = "") { return addUniform(type, name, qualifiers, true); }
    Shader::LocationHandle addFragmentUniform(std::string type, std::string name, std::string qualifiers = "") { return addUniform(type, name, qualifiers, false); }
    void addVarying(std::string type, std::string name, std::string qualifiers = "");
    void requireExtension(std::string extension);
    void requireLinearInterpolation() {
        if (usingGLES())
            requireExtension("GL_NV_shader_noperspective_interpolation");
    }
    int nextTextureSlot();
    void disableFragmentShader();

protected:
    virtual std::string buildVertexSource();
    virtual std::string buildFragmentSource();

private:
    std::vector<Shader::Location> locations;
    int contextCount = 0;
    int textureSlot = 0;
    bool fragmentShaderDisabled = false;

    friend Shader;
};

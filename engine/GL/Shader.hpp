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

    Shader() { }
    Shader(ShaderBuilder& builder) { initalize(builder); }

    void initalize(ShaderBuilder& builder);
    void use() const;
    bool isValid() const;
    void release();

    GLint getUniform(LocationHandle handle) const;
    GLuint getAttrib(LocationHandle handle) const;
    GLuint getAndEnableAttrib(LocationHandle handle) const;

    void uniform1f(LocationHandle uniform, float u) const;
    void uniform1i(LocationHandle uniform, int u) const;
    void uniform2f(LocationHandle uniform, float u1, float u2) const;
    void uniform2f(LocationHandle uniform, float* u) const;
    void uniform2i(LocationHandle uniform, int u1, int u2) const;
    void uniform2i(LocationHandle uniform, int* u) const;
    void uniform3f(LocationHandle uniform, float u1, float u2, float u3) const;
    void uniform3f(LocationHandle uniform, float* u) const;
    void uniform3i(LocationHandle uniform, int u1, int u2, int u3) const;
    void uniform3i(LocationHandle uniform, int* u) const;
    void uniform4f(LocationHandle uniform, float u1, float u2, float u3, float u4) const;
    void uniform4f(LocationHandle uniform, float* u) const;
    void uniform4i(LocationHandle uniform, int u1, int u2, int u3, int u4) const;
    void uniform4i(LocationHandle uniform, int* u) const;
    void uniformMatrix2f(LocationHandle uniform, float* u) const;
    void uniformMatrix3f(LocationHandle uniform, float* u) const;
    void uniformMatrix4f(LocationHandle uniform, float* u) const;
    void uniformVec2(LocationHandle uniform, Vec2 u) const;
    void uniformVec3(LocationHandle uniform, Vec3 u) const;
    void uniformVec4(LocationHandle uniform, Vec4 u) const;
    void uniformMat4x4(LocationHandle uniform, Mat4x4& u) const;
    void uniformTexture(LocationHandle uniform, Texture& u, GLuint slot) const;
    void uniformCubeMap(LocationHandle uniform, CubeMap& u, GLuint slot) const;

    void attribPointer(LocationHandle attrib, int size, void* a) const;
    void attribPointer(LocationHandle attrib, int size, int stride, void* a) const;
    void attribPointer(LocationHandle attrib, const VertexBuffer& a) const;
    void attribPointer(LocationHandle attrib, int stride, const VertexBuffer& a) const;

private:
    GLuint glId = 0;
    std::vector<GLuint> locations;
};

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

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

class OrderedShader;
class ShaderBuilder;

struct OrderedShaderState {
    GLuint attribIndex = 0;
    GLuint uniformIndex = 0;
    GLuint textureSlot = 0;
};

struct UniformBuffer {
    GBuffer buffer;
    int_t offset;
    int_t size;
};

class Shader {
public:
    struct LocationHandle {
        GLuint handle;
    };

    static const std::string FRAGMENT_COLOR;

    Shader() { }
    Shader(ShaderBuilder& builder) { initialize(builder); }

    void initialize(ShaderBuilder& builder);
    void use() const;
    bool isValid() const;
    void release();

    GLint getUniform(LocationHandle handle) const;
    GLuint getAttrib(LocationHandle handle) const;
    GLuint getAndEnableAttrib(LocationHandle handle) const;

    void uniform1f(LocationHandle uniform, float u) const;
    void uniform1i(LocationHandle uniform, int u) const;
    void uniform2f(LocationHandle uniform, float u1, float u2) const;
    void uniform2f(LocationHandle uniform, const float* u) const;
    void uniform2i(LocationHandle uniform, int u1, int u2) const;
    void uniform2i(LocationHandle uniform, const int* u) const;
    void uniform3f(LocationHandle uniform, float u1, float u2, float u3) const;
    void uniform3f(LocationHandle uniform, const float* u) const;
    void uniform3i(LocationHandle uniform, int u1, int u2, int u3) const;
    void uniform3i(LocationHandle uniform, const int* u) const;
    void uniform4f(LocationHandle uniform, float u1, float u2, float u3, float u4) const;
    void uniform4f(LocationHandle uniform, const float* u) const;
    void uniform4i(LocationHandle uniform, int u1, int u2, int u3, int u4) const;
    void uniform4i(LocationHandle uniform, const int* u) const;
    void uniformMatrix2f(LocationHandle uniform, const float* u) const;
    void uniformMatrix3f(LocationHandle uniform, const float* u) const;
    void uniformMatrix4f(LocationHandle uniform, const float* u) const;
    void uniformVec2(LocationHandle uniform, Vec2 u) const;
    void uniformVec3(LocationHandle uniform, Vec3 u) const;
    void uniformVec4(LocationHandle uniform, Vec4 u) const;
    void uniformMat4x4(LocationHandle uniform, const Mat4x4& u) const;
    void uniformTexture(LocationHandle uniform, const Texture& u, GLuint slot) const;
    void uniformCubeMap(LocationHandle uniform, const CubeMap& u, GLuint slot) const;

    void attribPointer(LocationHandle attrib, int size, const void* a) const;
    void attribPointer(LocationHandle attrib, int size, int stride, const void* a) const;
    void attribPointer(LocationHandle attrib, const VertexBuffer& a) const;
    void attribPointer(LocationHandle attrib, int stride, const VertexBuffer& a) const;

    OrderedShader ordered(OrderedShaderState state);

private:
    GLuint glId = 0;
};

class OrderedShader {
public:
    void uniform1f(float u) { shader.uniform1f({ state.uniformIndex++ }, u); }
    void uniform1i(int u) { shader.uniform1i({ state.uniformIndex++ }, u); }
    void uniform2f(float u1, float u2) { shader.uniform2f({ state.uniformIndex++ }, u1, u2); }
    void uniform2f(const float* u) { shader.uniform2f({ state.uniformIndex++ }, u); }
    void uniform2i(int u1, int u2) { shader.uniform2i({ state.uniformIndex++ }, u1, u2); }
    void uniform2i(const int* u) { shader.uniform2i({ state.uniformIndex++ }, u); }
    void uniform3f(float u1, float u2, float u3) { shader.uniform3f({ state.uniformIndex++ }, u1, u2, u3); }
    void uniform3f(const float* u) { shader.uniform3f({ state.uniformIndex++ }, u); }
    void uniform3i(int u1, int u2, int u3) { shader.uniform3i({ state.uniformIndex++ }, u1, u2, u3); }
    void uniform3i(const int* u) { shader.uniform3i({ state.uniformIndex++ }, u); }
    void uniform4f(float u1, float u2, float u3, float u4) { shader.uniform4f({ state.uniformIndex++ }, u1, u2, u3, u4); }
    void uniform4f(const float* u) { shader.uniform4f({ state.uniformIndex++ }, u); }
    void uniform4i(int u1, int u2, int u3, int u4) { shader.uniform4i({ state.uniformIndex++ }, u1, u2, u3, u4); }
    void uniform4i(const int* u) { shader.uniform4i({ state.uniformIndex++ }, u); }
    void uniformMatrix2f(const float* u) { shader.uniformMatrix2f({ state.uniformIndex++ }, u); }
    void uniformMatrix3f(const float* u) { shader.uniformMatrix3f({ state.uniformIndex++ }, u); }
    void uniformMatrix4f(const float* u) { shader.uniformMatrix4f({ state.uniformIndex++ }, u); }
    void uniformVec2(Vec2 u) { shader.uniformVec2({ state.uniformIndex++ }, u); }
    void uniformVec3(Vec3 u) { shader.uniformVec3({ state.uniformIndex++ }, u); }
    void uniformVec4(Vec4 u) { shader.uniformVec4({ state.uniformIndex++ }, u); }
    void uniformMat4x4(const Mat4x4& u) { shader.uniformMat4x4({ state.uniformIndex++ }, u); }
    void uniformTexture(const Texture& u) { shader.uniformTexture({ state.uniformIndex++ }, u, state.textureSlot++); }
    void uniformCubeMap(const CubeMap& u) { shader.uniformCubeMap({ state.uniformIndex++ }, u, state.textureSlot++); }

    void attribPointer(int size, const void* a) { shader.attribPointer({ state.attribIndex++ }, size, a); }
    void attribPointer(int size, int stride, const void* a) { shader.attribPointer({ state.attribIndex++ }, size, stride, a); }
    void attribPointer(const VertexBuffer& a) { shader.attribPointer({ state.attribIndex++ }, a); }
    void attribPointer(int stride, const VertexBuffer& a) { shader.attribPointer({ state.attribIndex++ }, stride, a); }

private:
    const Shader shader;
    OrderedShaderState state;

    OrderedShader(Shader shader, OrderedShaderState state)
        : shader(shader), state(state) { }

    friend Shader;
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
    OrderedShaderState orderedState() { return state; }

protected:
    virtual std::string buildVertexSource();
    virtual std::string buildFragmentSource();

private:
    int contextCount = 0;
    OrderedShaderState state;
    bool fragmentShaderDisabled = false;

    friend Shader;
};

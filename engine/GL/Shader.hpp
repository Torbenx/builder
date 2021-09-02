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
    GLuint uniformBufferIndex = 0;
};

struct UniformBuffer {
    GBuffer buffer;
    int_t offset;
    int_t size;
};

struct VertexAttribBuffer {
    GBuffer buffer;
    int_t offset;
    int_t elementSize;
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

    void use() const {
        VERIFY(GL, glIsProgram(glId));
        glUseProgram(glId);
    }
    bool isValid() const { return glId != 0; }
    void release() {
        VERIFY(GL, glIsProgram(glId));
        glDeleteProgram(glId);
        glId = 0;
    }

    GLuint enableAttrib(LocationHandle handle) const {
        GLuint a = handle.handle;
        glEnableVertexAttribArray(a);
        return a;
    }
    GLint getUniform(LocationHandle handle) const { return handle.handle; }

    void uniform1f(LocationHandle uniform, float u) const { glUniform1f(getUniform(uniform), u); }
    void uniform1i(LocationHandle uniform, GLint u) const { glUniform1i(getUniform(uniform), u); }
    void uniformVec2(LocationHandle uniform, Vec2 u) const { glUniform2f(getUniform(uniform), u.x, u.y); }
    void uniformVec3(LocationHandle uniform, Vec3 u) const { glUniform3f(getUniform(uniform), u.x, u.y, u.z); }
    void uniformVec4(LocationHandle uniform, Vec4 u) const { glUniform4f(getUniform(uniform), u.x, u.y, u.z, u.w); }
    void uniformMat4x4(LocationHandle uniform, const Mat4x4& u) const { glUniformMatrix4fv(getUniform(uniform), 1, GL_FALSE, u.matrix); }

    void uniformTexture(LocationHandle uniform, const Texture& u, GLuint slot) const {
        glActiveTexture(GL_TEXTURE0 + slot);
        u.bind();
        uniform1i(uniform, slot);
    }
    void uniformCubeMap(LocationHandle uniform, const CubeMap& u, GLuint slot) const { uniformTexture(uniform, u, slot); }
    void uniformBuffer(LocationHandle binding, const UniformBuffer& u) const {
        glBindBufferRange(GL_UNIFORM_BUFFER, binding.handle, u.buffer.glId, u.offset, u.size);
    }

    void attribPointer(LocationHandle attrib, const VertexBuffer& a) const {
        a.bind(GL_ARRAY_BUFFER);
        glVertexAttribPointer(enableAttrib(attrib), a.elementSize(), GL_FLOAT, GL_FALSE, 0, a.data());
    }
    void attribPointer(LocationHandle attrib, const VertexAttribBuffer& a) const {
        a.buffer.bind(GL_ARRAY_BUFFER);
        glVertexAttribPointer(enableAttrib(attrib), a.elementSize, GL_FLOAT, GL_FALSE, 0, (void*)a.offset);
    }

    OrderedShader ordered(OrderedShaderState state);

//private:
    GLuint glId = 0;
};

class OrderedShader {
public:
    void uniform1f(float u) { shader.uniform1f({ state.uniformIndex++ }, u); }
    void uniform1i(int u) { shader.uniform1i({ state.uniformIndex++ }, u); }
    void uniformVec2(Vec2 u) { shader.uniformVec2({ state.uniformIndex++ }, u); }
    void uniformVec3(Vec3 u) { shader.uniformVec3({ state.uniformIndex++ }, u); }
    void uniformVec4(Vec4 u) { shader.uniformVec4({ state.uniformIndex++ }, u); }
    void uniformMat4x4(const Mat4x4& u) { shader.uniformMat4x4({ state.uniformIndex++ }, u); }
    void uniformTexture(const Texture& u) { shader.uniformTexture({ state.uniformIndex++ }, u, state.textureSlot++); }
    void uniformCubeMap(const CubeMap& u) { shader.uniformCubeMap({ state.uniformIndex++ }, u, state.textureSlot++); }
    void uniformBuffer(const UniformBuffer& u) { shader.uniformBuffer({ state.uniformBufferIndex++ }, u); }

    void attribPointer(const VertexBuffer& a) { shader.attribPointer({ state.attribIndex++ }, a); }
    void attribPointer(const VertexAttribBuffer& a) { shader.attribPointer({ state.attribIndex++ }, a); }

    const Shader shader;
    OrderedShaderState state;
private:
    OrderedShader(Shader shader, OrderedShaderState state)
        : shader(shader), state(state) { }

    friend Shader;
};

inline OrderedShader Shader::ordered(OrderedShaderState state) { return OrderedShader(*this, state); }

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
    Shader::LocationHandle beginUniformBuffer(std::string name, bool vertex);
    Shader::LocationHandle beginVertexUniformBuffer(std::string name) { return beginUniformBuffer(name, true); }
    Shader::LocationHandle beginFragmentUniformBuffer(std::string name) { return beginUniformBuffer(name, false); }
    void endUniformBuffer(bool vertex);
    void endVertexUniformBuffer() { endUniformBuffer(true); }
    void endFragmentUniformBuffer() { endUniformBuffer(false); }
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

#pragma once

#include "GL/Shader.hpp"
#include "util/text.hpp"
#include "util/log.hpp"
#include <memory>

struct SimpleTextureShader : public Shader{
    LocationHandle positionAttrib, texCoordAttrib, textureUniform;
    void applySimpleTextureShader(ShaderBuilder& b, const std::string& sampler);
};

struct SimpleTextShader : public SimpleTextureShader{
    void applySimpleTextShader(ShaderBuilder& b);
};

struct SimpleDepthShader : public Shader{
    LocationHandle positionAttrib, mvpUniform, modelUniform, lightPosUniform, farUniform;
    void applySimpleDepthShader(ShaderBuilder& b);
};

struct SingleColorShader : public Shader{
    LocationHandle positionAttrib, colorUniform;
    void applySingleColorShader(ShaderBuilder& b);
};

struct SimpleCubeMapShader : public Shader{
    LocationHandle positionAttrib, texCoordAttrib, cubeMapUniform;
    void applySimpleCubeMapShader(ShaderBuilder& b, const std::string& sampler);
};

struct SimpleDepthMapShader : public SimpleTextureShader{
    LocationHandle farUniform;
    void applySimpleDepthMapShader(ShaderBuilder& b);
};

struct SimpleDepthCubeMapShader : public SimpleCubeMapShader{
    LocationHandle farUniform;
    void applySimpleDepthCubeMapShader(ShaderBuilder& b);
};

#define DECLARE_SHADER(name, type, ...) \
static inline std::weak_ptr<type> w_##name; \
static std::shared_ptr<type> name(){ \
    if(w_##name.expired()){ \
        LOGD("constructing Shaders::" #name); \
        ShaderBuilder b; \
        auto p = std::make_shared<type>(); \
        p->apply##type(b __VA_OPT__(,) __VA_ARGS__); \
        p->initalize(b); \
        w_##name = p; \
        return p; \
    } \
    return w_##name.lock(); \
} \

class Shaders{
public:
    DECLARE_SHADER(texture, SimpleTextureShader, "sampler2D")
    DECLARE_SHADER(depth, SimpleDepthShader)
    DECLARE_SHADER(text, SimpleTextShader)
    DECLARE_SHADER(color, SingleColorShader)
    DECLARE_SHADER(cubeMap, SimpleCubeMapShader, "samplerCube")
    DECLARE_SHADER(depthMap, SimpleDepthMapShader)
    DECLARE_SHADER(depthCubeMap, SimpleDepthCubeMapShader)
private:
    Shaders();
};

#pragma once

#include "GL/Shader.hpp"
#include "resource/Resource.hpp"
#include "util/log.hpp"
#include "util/text.hpp"

struct SimpleTextureShader : public Shader {
    LocationHandle positionAttrib, texCoordAttrib, textureUniform;
    void applySimpleTextureShader(ShaderBuilder& b, const std::string& sampler = "sampler2D");
};

struct SimpleTextShader : public SimpleTextureShader {
    void applySimpleTextShader(ShaderBuilder& b);
};

struct SimpleDepthShader : public Shader {
    LocationHandle positionAttrib, mvpUniform;
    void applySimpleDepthShader(ShaderBuilder& b);
};

struct SingleColorShader : public Shader {
    LocationHandle positionAttrib, colorUniform;
    void applySingleColorShader(ShaderBuilder& b);
};

struct SimpleCubeMapShader : public Shader {
    LocationHandle positionAttrib, texCoordAttrib, cubeMapUniform;
    void applySimpleCubeMapShader(ShaderBuilder& b, const std::string& sampler = "samplerCube");
};

#define DECLARE_SHADER(name, type)                \
    static inline Resource<type>::Weak w_##name;  \
    static Resource<type> name() {                \
        if (w_##name.expired()) {                 \
            LOGD("constructing Shaders::" #name); \
            ShaderBuilder b;                      \
            type s = type();                      \
            s.apply##type(b);                     \
            s.initalize(b);                       \
            Resource<type> r(s);                  \
            w_##name = r.weak();                  \
            return r;                             \
        }                                         \
        return w_##name.acquire();                \
    }

class Shaders {
public:
    DECLARE_SHADER(texture, SimpleTextureShader)
    DECLARE_SHADER(depth, SimpleDepthShader)
    DECLARE_SHADER(text, SimpleTextShader)
    DECLARE_SHADER(color, SingleColorShader)
    DECLARE_SHADER(cubeMap, SimpleCubeMapShader)
private:
    Shaders();
};

#pragma once

#include "GL/Shader.hpp"
#include "resource/Resource.hpp"
#include "util/log.hpp"
#include "util/text.hpp"

template<typename T>
struct StaticResourceMonitor final : public ResourceMonitor {
    T value;

    Resource<T> acquire() {
        if (!expired()) {
            return Resource<T>(value, this);
        } else {
            return Resource<T>();
        }
    }

    Resource<T> set(T new_value) {
        if (!expired()) {
            LOGD("StaticResourceMonitor::set on resource that hasn't expired!");
        }
        const_cast<T&>(value) = new_value;
        return Resource<T>(value, this);
    }

    bool release() override {
        return true;
    }
};

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

#define DECLARE_SHADER(name, type)                      \
    static inline StaticResourceMonitor<type> m_##name; \
    static Resource<type> name() {                      \
        if (!m_##name.expired()) {                      \
            return m_##name.acquire();                  \
        }                                               \
        LOGD("constructing Shaders::" #name);           \
        ShaderBuilder b;                                \
        type s = type();                                \
        s.apply##type(b);                               \
        s.initialize(b);                                \
        return m_##name.set(s);                         \
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

#pragma once

#include "GL/Shader.hpp"

class LShaderBuilder;

class LShader : public Shader {
public:
    static const std::string WORLD_POSITION;
    static const std::string WORLD_NORMAL;
    static const std::string SCREEN_POSITION;
    static const std::string LIGHT_INTENSITY;

    struct EffectData {
        LocationHandle handles[5];
        int textureSlot;
    };

    struct Effect {
        virtual void bindTo(LShader& shader, const EffectData& data) = 0;
        virtual ~Effect() { }
    };

    struct EffectLocationHandle {
        int handle;
    };

    void bindEffect(EffectLocationHandle handle, Effect& effect);

    LShader(LShaderBuilder& builder);

private:
    std::vector<EffectData> data;
};

class LShaderBuilder : public ShaderBuilder {
public:
    std::string fragmentColor, fragmentDiffuseColor;
    std::string rawPosition, rawNormal;
    std::string rawToWorldMatrix, worldToScreenMatrix;

    LShaderBuilder();

    LShader::EffectLocationHandle addEffect(const LShader::EffectData& data);

protected:
    virtual std::string buildVertexSource() override;
    virtual std::string buildFragmentSource() override;

private:
    std::vector<LShader::EffectData> data;
    friend LShader;
};

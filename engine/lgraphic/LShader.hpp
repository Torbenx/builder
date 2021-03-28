#ifndef HEADER_LGRAPHIC_LSHADER_H
#define HEADER_LGRAPHIC_LSHADER_H

#include "../graphic/Shader.hpp"

class LShaderBuilder;

class LShader : public Shader{
public:
    static const std::string WORLD_POSITION;
    static const std::string WORLD_NORMAL;
    static const std::string SCREEN_POSITION;
    static const std::string LIGHT_INTENSITY;

    struct EffectData{
        LocationHandle handles[5];
        int textureSlot;
    };

    class Effect{
    public:
        virtual void apply(LShader& shader, const EffectData& data) = 0;
        virtual ~Effect() {};
    };

    struct EffectAndData{
        Effect *effect;
        EffectData data;
    };

    LShader(LShaderBuilder& builder);
    void applyEffects();

private:
    std::vector<EffectAndData> data;
};

class LShaderBuilder : public ShaderBuilder{
public:
    std::string fragmentColor, fragmentDiffuseColor;
    std::string rawPosition, rawNormal;
    std::string rawToWorldMatrix, worldToScreenMatrix;

    LShaderBuilder();

    void addEffect(LShader::Effect&, const LShader::EffectData& data);

protected:
    virtual std::string buildVertexSource() override;
    virtual std::string buildFragmentSource() override;

private:
    std::vector<LShader::EffectAndData> data;
    friend LShader;
};

#endif

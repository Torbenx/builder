#ifndef HEADER_LGRAPHIC_LSHADER_H
#define HEADER_LGRAPHIC_LSHADER_H

#include "../graphic/Shader.hpp"

class LShader : public Shader{
public:

    static const std::string WORLD_POSITION;
    static const std::string WORLD_NORMAL;
    static const std::string SCREEN_POSITION;
    static const std::string LIGHT_INTENSITY;

    struct EffectData;
    class Effect{
    public:
        virtual void apply(LShader& shader, const EffectData& data) = 0;
        virtual ~Effect() {};
    };
    struct EffectData{
        LocationHandle handles[5];
        int textureSlot;
    };

    std::string fragmentColor, fragmentDiffuseColor, rawPosition, rawNormal;
    std::string rawToWorldMatrix, worldToScreenMatrix;
    LShader();
    virtual std::string buildVertexSource() override;
    virtual std::string buildFragmentSource() override;
    void applyEffects();
    void addEffect(Effect&, const EffectData& data);

private:
    struct EffectAndData{
        Effect *effect;
        EffectData data;
    };
    std::vector<EffectAndData> data;
};

#endif

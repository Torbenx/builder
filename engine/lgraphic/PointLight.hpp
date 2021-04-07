#pragma once

#include "lgraphic/DepthCubeMap.hpp"
#include "lgraphic/LShader.hpp"

class PointLight : public LShader::Effect {
public:
    Vec3 position;
    DepthCubeMap depthMap;
    Vec3 color;
    PointLight(Vec3 position, int width, int height, Vec3 color, float near, float far);
    void draw(void (*doDraw)(Framebuffer& framebuffer, Shader& shader, Shader::LocationHandle positionAttrib, Shader::LocationHandle modelUniform));
    static LShader::EffectLocationHandle addTo(LShaderBuilder&);
    virtual void bindTo(LShader&, const LShader::EffectData&) override;
};

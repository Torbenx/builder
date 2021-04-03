#pragma once

#include "GL/Framebuffer.hpp"
#include "lgraphic/LShader.hpp"

class Reflection : public LShader::Effect{
    public:
        Framebuffer framebuffer;
        Texture depthAttachment;
        Texture texture;
        Vec3 reflection, position;
        float factor;
        Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor);
        Mat4x4 getReflectionMatrix();
        void addTo(LShaderBuilder& s);
        virtual void apply(LShader&, const  LShader::EffectData&) override;
};

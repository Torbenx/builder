#ifndef HEADER_LGRAPHIC_REFLECTION_REFLECTION_H
#define HEADER_LGRAPHIC_REFLECTION_REFLECTION_H

#include "../../graphic/Framebuffer.hpp"
#include "../../math/MvpMatrix.hpp"
#include "../LShader.hpp"

class Reflection : public LShader::Effect{
    public:
        Framebuffer framebuffer;
        Texture depthAttachment;
        Texture texture;
        Vec3 reflection, position;
        float factor;
        Reflection(Vec3 reflection, Vec3 position, int width, int height, float factor);
        void getReflectionModel(Mat4x4& model);
        void addTo(LShaderBuilder& s);
        virtual void apply(LShader&, const  LShader::EffectData&) override;
};

#endif

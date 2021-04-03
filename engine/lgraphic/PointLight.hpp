#pragma once

#include "lgraphic/DepthCubeMap.hpp"
#include "lgraphic/LShader.hpp"

class PointLight : public LShader::Effect{
    public:
        Vec3 position;
        DepthCubeMap depthMap;
        Vec3 color;
        PointLight(Vec3 position,int width,int height,Vec3 color,float near,float far);
        void draw(Renderable **ptrArray, int count);
        void addTo(LShaderBuilder&);
        virtual void apply(LShader&, const  LShader::EffectData&) override;
};

#ifndef HEADER_LGRAPHIC_LIGHT_SPOTLIGHT_H
#define HEADER_LGRAPHIC_LIGHT_SPOTLIGHT_H

#include "../DepthMap.hpp"
#include "../LShader.hpp"
#include "../../math/MvpMatrix.hpp"

class SpotLight : public LShader::Effect{
    public:
        Vec3 direction,position,hDirection;
        DepthMap depthMap;
        float hfov,vfov;
        MvpMatrix mvpMatrix;
        Vec3 color;
        SpotLight(Vec3 position,Vec3 direction,float fov,int texWidth,int texHeight,Vec3 color,float near,float far);
        SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,Vec3 hDirection,int texWidth,int texHeight,Vec3 color,float near,float far);
        ~SpotLight();
        void draw(Renderable **ptrArray, int count);
        void addTo(LShaderBuilder&);
        virtual void apply(LShader&, const  LShader::EffectData&) override;
};

#endif

#ifndef HEADER_LGRAPHIC_LIGHT_POINTLIGHT
#define HEADER_LGRAPHIC_LIGHT_POINTLIGHT

#include "../DepthCubeMap.hpp"
#include "../LShader.hpp"

class PointLight : public LShader::Effect{
    public:
        Vec3 position;
        DepthCubeMap depthMap;
        Vec3 color;
        PointLight(Vec3 position,int width,int height,Vec3 color,float near,float far);
        ~PointLight();
        void init();
        void draw(Renderable **ptrArray, int count);
        void addTo(LShaderBuilder&);
        virtual void apply(LShader&, const  LShader::EffectData&) override;
};

#endif

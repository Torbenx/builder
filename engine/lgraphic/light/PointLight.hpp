#ifndef HEADER_LGRAPHIC_LIGHT_POINTLIGHT
#define HEADER_LGRAPHIC_LIGHT_POINTLIGHT

#include "../DepthCubeMap.hpp"

class PointLight{
    public:
        Vec3 position;
        DepthCubeMap depthMap;
        Vec3 color;
        float halfBrightness,startBrightness;
        PointLight(Vec3 position,int width,int height,Vec3 color,float near,float far);
        ~PointLight();
        void init();
        void draw(Mat4x4 model, Renderable **ptrArray, int count);
};

#endif

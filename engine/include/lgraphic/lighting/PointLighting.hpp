#ifndef HEADER_LGRAPHIC_LIGHTING_POINTLIGHTING
#define HEADER_LGRAPHIC_LIGHTING_POINTLIGHTING

#include "../DepthCubeMap.hpp"

class PointLighting{
    public:
        Vec3 position;
        DepthCubeMap *depthMap;
        Vec3 color;
        float halfBrightness,startBrightness;
        PointLighting(Vec3 position,int width,int height,Vec3 color,float far,float startBrightness,float halfBrightness);
        ~PointLighting();
        void init();
        void draw(Mat4x4 model,Renderable **ptrArray,int count);
};

#endif

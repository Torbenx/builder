#ifndef HEADER_LGRAPHIC_LIGHTING_SPOTLIGHTING_H
#define HEADER_LGRAPHIC_LIGHTING_SPOTLIGHTING_H

#include "../DepthMap.hpp"

class SpotLighting{
    public:
        Vec3 position,direction,up;
        DepthMap *depthMap;
        float hfov,vfov;
        MvpMatrix mvpMatrix;
        int width,height;
        Vec3 color;
        SpotLighting(Vec3 position,Vec3 direction,float hfov,float vfov,int texWidth,int texHeight,Vec3 color,float far);
        ~SpotLighting();
        void init();
        void draw(Mat4x4 model,Renderable **ptrArray,int count);
};

#endif

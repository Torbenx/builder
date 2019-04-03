#ifndef HEADER_LGRAPHIC_LIGHT_SPOTLIGHT_H
#define HEADER_LGRAPHIC_LIGHT_SPOTLIGHT_H

#include "../DepthMap.hpp"

class SpotLight{
    public:
        Vec3 position,direction,up;
        DepthMap depthMap;
        float hfov,vfov;
        MvpMatrix mvpMatrix;
        int width,height;
        Vec3 color;
        SpotLight(Vec3 position,Vec3 direction,float hfov,float vfov,int texWidth,int texHeight,Vec3 color,float far);
        ~SpotLight();
        void init();
        void draw(Mat4x4 model, Renderable **ptrArray, int count);
};

#endif

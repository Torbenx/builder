#ifndef HEADER_UI_UICOMPONENT_H
#define HEADER_UI_UICOMPONENT_H

#include "../math/Rect.hpp"
#include <tgmath.h>

class UiComponent{
    public:
        Rect rect;
        int screenWidth;
        int screenHeight;
        virtual void updateRect() = 0;
        virtual void draw() = 0;
        virtual ~UiComponent(){};
        int toPixelWidth(float width){return (int)ceil(width*0.5f*screenWidth);}
        int toPixelHeight(float height){return (int)ceil(height*0.5f*screenHeight);}
        float toGLWidth(float pixelWidth){return pixelWidth * 2.0f / screenWidth;}
        float toGLHeight(float pixelHeight){return pixelHeight * 2.0f / screenHeight;}
};

#endif
#ifndef HEADER_UI_UICOMPONENT_H
#define HEADER_UI_UICOMPONENT_H

#include "Boundary.hpp"
#include "../math/Mat4x4.hpp"

class UiComponent{
    public:
        virtual void setTransform(Mat4x4 transform) = 0;
        virtual void draw() = 0;
        virtual Boundary getBoundary() = 0;
        virtual ~UiComponent(){};
        
};

#endif
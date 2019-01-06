#ifndef HEADER_UI_LAYOUT_H
#define HEADER_UI_LAYOUT_H

#include <list>
#include "UiComponent.hpp"

typedef struct{
    UiComponent *component;
    Vec2 translation;
    Vec2 scale;
} UiComponentData;

typedef UiComponentData* UiComponentHandle;

class Layout{
    public:
        std::list<UiComponentData> components;
        UiComponentHandle add(UiComponent *component);

        void setBoundary(UiComponentHandle uc, Boundary boundary);
        
        void width(UiComponentHandle uc, float width);
        void height(UiComponentHandle uc, float height);
        void widthPreserveAspect(UiComponentHandle uc, float width);
        void heightPreserveAspect(UiComponentHandle uc, float height);
        
        void top(UiComponentHandle uc, float top);
        void bottom(UiComponentHandle uc, float bottom);
        void left(UiComponentHandle uc, float left);
        void right(UiComponentHandle uc, float right);
        void above(UiComponentHandle below, UiComponentHandle above, float distance);
        void below(UiComponentHandle above, UiComponentHandle below, float distance);
        void rightOf(UiComponentHandle left, UiComponentHandle right, float distance);
        void leftOf(UiComponentHandle right, UiComponentHandle left, float distance);
        
        void setTransform(Mat4x4 transform);
        void draw();
        
        ~Layout();
};

#endif
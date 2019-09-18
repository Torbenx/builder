#ifndef HEADER_UI_UICOMPONENT_H
#define HEADER_UI_UICOMPONENT_H

#include "TypedSize.hpp"
#include "../math/Rect.hpp"
#include <tgmath.h>
#include <cmath>

class UiComponent{
    public:
        TypedSizeGetter getContentWidthGetter();
        TypedSizeGetter getContentHeightGetter();
        Rect rect;
        ScreenMetrics metrics;
        virtual void updateContent(){}
        virtual TypedSize getContentWidth(){return TypedSize::nan;}
        virtual TypedSize getContentHeight(){return TypedSize::nan;}
        virtual void updateRect() = 0;
        virtual void draw() = 0;
        virtual ~UiComponent(){}
};

#endif
#ifndef HEADER_UI_UICOMPONENT_H
#define HEADER_UI_UICOMPONENT_H

#include "TypedSize.hpp"
#include "../math/Rect.hpp"
#include "UiSolver.hpp"
#include <tgmath.h>
#include <cmath>

class UiComponent{
    public:
        Rect rect;
        ScreenMetrics metrics;
        ui_var_t base = UI_VAR_INVALID;
        
        virtual void updateContent(){}
        virtual TypedSizeXY getContentWidth(){return TypedSize::nan.asXSize();}
        virtual TypedSizeXY getContentHeight(){return TypedSize::nan.asYSize();}
        virtual void updateRect() = 0;
        virtual void draw() = 0;
        virtual ~UiComponent(){}
        
        LazyTypedSizeXY getContentWidthLazy();
        LazyTypedSizeXY getContentHeightLazy();
};

#endif
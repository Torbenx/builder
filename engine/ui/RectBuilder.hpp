#ifndef HEADER_UI_RECT_BUILDER_H
#define HEADER_UI_RECT_BUILDER_H

#include "../math/Rect.hpp"
#include "TypedSize.hpp"
#include <cmath>
#include <string>

struct RectX{
    void *rect;
    bool baseLeft;
    TypedSize dx;
};

struct RectY{
    void *rect;
    bool baseBottom;
    TypedSize dy;
};

union RectBuilderData{
    void *rect;
    RectX relx;
    RectY rely;
    struct{
        void *rect;
        void *thiz;
        float fract;
    } cen;
    struct{
        void *rect;
        bool baseLeft;
        TypedSizeGetter dx;
    } lazyx;
    struct{
        void *rect;
        bool baseBottom;
        TypedSizeGetter dy;
    } lazyy;
};

class RectBuilder{
    public:
        std::string name = "";
        Rect parent = Rect::bltr(-1.0f, -1.0f, 1.0f, 1.0f);
        
        RectY(*topF)(RectBuilderData data, ScreenMatrics m) = 0;
        RectY(*bottomF)(RectBuilderData data, ScreenMatrics m) = 0;
        RectX(*leftF)(RectBuilderData data, ScreenMatrics m) = 0;
        RectX(*rightF)(RectBuilderData data, ScreenMatrics m) = 0;
        RectBuilderData topD,bottomD,leftD,rightD;
        bool useTopForHeight = false;
        bool useBottomForHeight = false;
        bool useLeftForWidth = false;
        bool useRightForWidth = false;
        
        void above(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void below(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void toLeftOf(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void toRightOf(RectBuilder *b, TypedSize padding=TypedSize::zero);
        
        void height(TypedSize height);
        void width(TypedSize width);
        
        void width(TypedSizeGetter width);
        void height(TypedSizeGetter height);
        
        void alignTop(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void alignBottom(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void alignLeft(RectBuilder *b, TypedSize padding=TypedSize::zero);
        void alignRight(RectBuilder *b, TypedSize padding=TypedSize::zero);
        
        void centerVerticalInside(RectBuilder *b, float fraction=0.5f);
        void centerHorizontalInside(RectBuilder *b, float fraction=0.5f);
        
        float getWidth(ScreenMatrics m);
        float getHeight(ScreenMatrics m);
        float getTop(ScreenMatrics m);
        float getBottom(ScreenMatrics m);
        float getLeft(ScreenMatrics m);
        float getRight(ScreenMatrics m);
        Rect build(ScreenMatrics m);
};

#endif
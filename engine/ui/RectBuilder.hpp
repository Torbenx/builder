#ifndef HEADER_UI_RECT_BUILDER_H
#define HEADER_UI_RECT_BUILDER_H

#include "../math/Rect.hpp"
#include <cmath>
#include <string>

struct RectX{
    void *rect;
    bool baseLeft;
    float dx;
};

struct RectY{
    void *rect;
    bool baseBottom;
    float dy;
};

union RectBuilderData{
    RectX relx;
    RectY rely;
    struct{
        void *thiz;
        void *rect;
        float fract;
    } cen;
};

struct RectBuilder{
    std::string name = "";
    RectY(*topF)(RectBuilderData data) = 0;
    RectY(*bottomF)(RectBuilderData data) = 0;
    RectX(*leftF)(RectBuilderData data) = 0;
    RectX(*rightF)(RectBuilderData data) = 0;
    RectBuilderData topD,bottomD,leftD,rightD;
    bool useTopForHeight = false;
    bool useBottomForHeight = false;
    bool useLeftForWidth = false;
    bool useRightForWidth = false;
    
    void above(RectBuilder *b, float padding=0.0f);
    void below(RectBuilder *b, float padding=0.0f);
    void toLeftOf(RectBuilder *b, float padding=0.0f);
    void toRightOf(RectBuilder *b, float padding=0.0f);
    
    void height(float height);
    void width(float width);
    
    void alignTop(RectBuilder *b, float padding=0.0f);
    void alignBottom(RectBuilder *b, float padding=0.0f);
    void alignLeft(RectBuilder *b, float padding=0.0f);
    void alignRight(RectBuilder *b, float padding=0.0f);
    
    void centerVerticalInside(RectBuilder *b, float fraction=0.5f);
    void centerHorizontalInside(RectBuilder *b, float fraction=0.5f);
    
    float getWidth();
    float getHeight();
    float getTop();
    float getBottom();
    float getLeft();
    float getRight();
    Rect build();
};

#endif
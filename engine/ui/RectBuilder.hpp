#ifndef HEADER_UI_RECT_BUILDER_H
#define HEADER_UI_RECT_BUILDER_H

#include "../math/Rect.hpp"
#include <cmath>

union RectBuilderData{
    float f;
    struct{
        void *b;
        float pad;
    } rel;
    struct{
        void *thiz;
        float big,small;
    } cenFF;
    struct{
        void *thiz;
        float big;
        void *small;
    } cenFB;
    struct{
        void *thiz;
        void *big;
        float small;
    } cenBF;
    struct{
        void *thiz;
        void *big,*small;
    } cenBB;
};

struct RectBuilder{
    float(*widthF)(RectBuilderData data) = 0;
    float(*heightF)(RectBuilderData data) = 0;
    float(*topF)(RectBuilderData data) = 0;
    float(*bottomF)(RectBuilderData data) = 0;
    float(*leftF)(RectBuilderData data) = 0;
    float(*rightF)(RectBuilderData data) = 0;
    RectBuilderData widthD,heightD,topD,bottomD,leftD,rightD;
    float cwidth = NAN,cheight = NAN,ctop = NAN,cbottom = NAN,cleft = NAN,cright = NAN;
    
    void above(RectBuilder *b, float padding=0.0f);
    void below(RectBuilder *b, float padding=0.0f);
    void toLeftOf(RectBuilder *b, float padding=0.0f);
    void toRightOf(RectBuilder *b, float padding=0.0f);
    
    void top(float top);
    void bottom(float bottom);
    void left(float left);
    void right(float right);
    void width(float width);
    void height(float height);
    
    void alignTop(RectBuilder *b, float padding=0.0f);
    void alignBottom(RectBuilder *b, float padding=0.0f);
    void alignLeft(RectBuilder *b, float padding=0.0f);
    void alignRight(RectBuilder *b, float padding=0.0f);
    
    void centerVertical(float above, float below);
    void centerVertical(RectBuilder *above, float below);
    void centerVertical(float above, RectBuilder *below);
    void centerVertical(RectBuilder *above, RectBuilder *below);
    void centerHorizontal(float left, float right);
    void centerHorizontal(RectBuilder *left, float right);
    void centerHorizontal(float left, RectBuilder *right);
    void centerHorizontal(RectBuilder *left, RectBuilder *right);
    
    float getWidth();
    float getHeight();
    float getTop();
    float getBottom();
    float getLeft();
    float getRight();
    Rect build();
};

#endif
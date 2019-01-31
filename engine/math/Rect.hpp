#ifndef HEADER_MATH_RECT_H
#define HEADER_MATH_RECT_H

#include "Vec2.hpp"

class Rect{
    public:
        Vec2 tl;
        Vec2 br;
        Rect();
        float getWidth();
        float getHeight();
        float getTop();
        float getLeft();
        float getBottom();
        float getRight();
        Vec2 getTopLeft();
        Vec2 getTopRight();
        Vec2 getBottomRight();
        Vec2 getBottomLeft();
        Rect operator+=(Vec2 right);
        Rect operator-=(Vec2 right);
        Rect operator=(Rect right);
        static Rect btlr(float bottom, float top, float left, float right);
        static Rect bltr(float bottom, float left, float top, float right);
        static Rect tlbr(float top, float left, float bottom, float right);
};

#endif
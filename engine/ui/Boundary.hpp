#ifndef HEADER_UI_BOUNDARY_H
#define HEADER_UI_BOUNDARY_H

#include "../math/Vec2.hpp"

typedef struct{
    Vec2 tl;
    Vec2 br;
    float getTop(){return tl.y;}
    float getLeft(){return tl.x;}
    float getBottom(){return br.y;}
    float getRight(){return br.x;}
    Vec2 getTopLeft(){return Vec2(getLeft(), getTop());}
    Vec2 getTopRight(){return Vec2(getRight(), getTop());}
    Vec2 getBottomRight(){return Vec2(getRight(), getBottom());}
    Vec2 getBottomLeft(){return Vec2(getLeft(), getBottom());}
    float getWidth(){return getRight() - getLeft();}
    float getHeight(){return getTop() - getBottom();}
} Rect;

typedef Rect Boundary;

#endif
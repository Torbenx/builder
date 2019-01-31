#include "Rect.hpp"

Rect::Rect(){
    
}

float Rect::getWidth(){
    return br.x - tl.x;
}

float Rect::getHeight(){
    return tl.y - br.y;
}

float Rect::getTop(){
    return tl.y;
}

float Rect::getBottom(){
    return br.y;
}

float Rect::getLeft(){
    return tl.x;
}

float Rect::getRight(){
    return br.x;
}

Vec2 Rect::getTopLeft(){
    return tl;
}

Vec2 Rect::getTopRight(){
    return Vec2(br.x, tl.y);
}

Vec2 Rect::getBottomRight(){
    return br;
}

Vec2 Rect::getBottomLeft(){
    return Vec2(tl.x, br.y);
}

Rect Rect::operator+=(Vec2 right){
    tl += right;
    br += right;
    return *this;
}

Rect Rect::operator-=(Vec2 right){
    tl -= right;
    br -= right;
    return *this;
}

Rect Rect::operator=(Rect other){
    tl = other.tl;
    br = other.br;
    return *this;
}

Rect Rect::btlr(float bottom, float top, float left, float right){
    Rect rect;
    rect.tl = Vec2(left, top);
    rect.br = Vec2(right, bottom);
    return rect;
}

Rect Rect::bltr(float bottom, float left, float top, float right){
    Rect rect;
    rect.tl = Vec2(left, top);
    rect.br = Vec2(right, bottom);
    return rect;
}

Rect Rect::tlbr(float top, float left, float bottom, float right){
    Rect rect;
    rect.tl = Vec2(left, top);
    rect.br = Vec2(right, bottom);
    return rect;
}
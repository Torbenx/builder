#include "Rect.hpp"
#include "../util/log.hpp"

Rect::Rect(){
    
}

float Rect::getWidth(){
    return right - left;
}

float Rect::getHeight(){
    return top - bottom;
}

float Rect::getTop(){
    return top;
}

float Rect::getBottom(){
    return bottom;
}

float Rect::getLeft(){
    return left;
}

float Rect::getRight(){
    return right;
}

Vec2 Rect::getTopLeft(){
    return Vec2(left, top);
}

Vec2 Rect::getTopRight(){
    return Vec2(right, top);
}

Vec2 Rect::getBottomRight(){
    return Vec2(right, bottom);
}

Vec2 Rect::getBottomLeft(){
    return Vec2(left, bottom);
}

bool Rect::contains(Vec2 p){
    return left <= p.x && p.x <= right && bottom <= p.y && p.y <= top;
}

void Rect::debug(){
    LOGD("top=%f, bottom=%f, left=%f, right=%f", top, bottom, left, right);
}

Rect Rect::operator+=(Vec2 v){
    top += v.y;
    bottom += v.y;
    left += v.x;
    right += v.x;
    return *this;
}

Rect Rect::operator-=(Vec2 v){
    top -= v.y;
    bottom -= v.y;
    left -= v.x;
    right -= v.x;
    return *this;
}

Rect Rect::operator=(Rect other){
    top = other.top;
    bottom = other.bottom;
    left = other.left;
    right = other.right;
    return *this;
}

Rect Rect::btlr(float bottom, float top, float left, float right){
    Rect rect;
    rect.top = top;
    rect.bottom = bottom;
    rect.left = left;
    rect.right = right;
    return rect;
}

Rect Rect::bltr(float bottom, float left, float top, float right){
    Rect rect;
    rect.top = top;
    rect.bottom = bottom;
    rect.left = left;
    rect.right = right;
    return rect;
}

Rect Rect::tlbr(float top, float left, float bottom, float right){
    Rect rect;
    rect.top = top;
    rect.bottom = bottom;
    rect.left = left;
    rect.right = right;
    return rect;
}
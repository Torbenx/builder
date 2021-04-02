#include "math/Vec2.hpp"
#include "util/log.hpp"
#include <cmath>

Vec2::Vec2(){
    set(0,0);
}

Vec2::Vec2(const Vec2& xy){
    set(xy.x,xy.y);
}

Vec2::Vec2(float xy){
    set(xy);
}

Vec2::Vec2(float x,float y){
    set(x,y);
}

void Vec2::set(float newX,float newY){
    x = newX;
    y = newY;
}

void Vec2::set(float xy){
    set(xy,xy);
}

void Vec2::set(Vec2& xy){
    set(xy.x,xy.y);
}

Vec2 Vec2::floor(){
    x = ::floor(x);
    y = ::floor(y);
    return *this;
}

Vec2 Vec2::ceil(){
    x = ::ceil(x);
    y = ::ceil(y);
    return *this;
}

Vec2 Vec2::fract(){
    x = x-::floor(x);
    y = y-::floor(y);
    return *this;
}

float Vec2::length(){
    return sqrt(dot(*this,*this));
}

Vec2 Vec2::normalize(){
    *this *= 1.0f/length();
    return *this;
}

Vec2 Vec2::operator+=(Vec2 right){
    this->x += right.x;
    this->y += right.y;
    return *this;
}

Vec2 Vec2::operator+=(float right){
    this->x += right;
    this->y += right;
    return *this;
}

Vec2 Vec2::operator-=(Vec2 right){
    this->x -= right.x;
    this->y -= right.y;
    return *this;
}

Vec2 Vec2::operator-=(float right){
    this->x -= right;
    this->y -= right;
    return *this;
}

Vec2 Vec2::operator*=(Vec2 right){
    this->x *= right.x;
    this->y *= right.y;
    return *this;
}

Vec2 Vec2::operator*=(float right){
    this->x *= right;
    this->y *= right;
    return *this;
}

Vec2 Vec2::operator/=(Vec2 right){
    this->x /= right.x;
    this->y /= right.y;
    return *this;
}

Vec2 Vec2::operator/=(float right){
    this->x /= right;
    this->y /= right;
    return *this;
}

Vec2 Vec2::operator=(Vec2 right){
    set(right);
    return *this;
}

void Vec2::debug(){
    LOGD("Vec2(%f,%f)",x,y);
}

float Vec2::dot(Vec2 left,Vec2 right){
    return left.x*right.x+left.y*right.y;
}

Vec2 operator+(Vec2 left,Vec2 right){
    return Vec2(left.x+right.x,left.y+right.y);
}

Vec2 operator+(Vec2 left,float right){
    return Vec2(left.x+right,left.y+right);
}

Vec2 operator*(Vec2 left,Vec2 right){
    return Vec2(left.x*right.x,left.y*right.y);
}

Vec2 operator*(Vec2 left,float right){
    return Vec2(left.x*right,left.y*right);
}

Vec2 operator*(float left,Vec2 right){
    return right*left;
}

Vec2 operator/(Vec2 left,Vec2 right){
    return Vec2(left.x/right.x,left.y/right.y);
}

Vec2 operator/(Vec2 left,float right){
    return Vec2(left.x/right,left.x/right);
}

Vec2 operator-(Vec2 left,Vec2 right){
    return Vec2(left.x-right.x,left.y-right.y);
}

Vec2 operator-(Vec2 left,float right){
    Vec2 re(left.x-right,left.y-right);
    return re;
}

Vec2 operator-(Vec2 v){
   return Vec2(-v.x, -v.y);
}

Vec2 operator+(Vec2 v){
    return v;
}

bool operator==(Vec2 left,Vec2 right){
    return left.x==right.x&&left.y==right.y;
}

bool operator!=(Vec2 left,Vec2 right){
    return !(left==right);
}

#include "Vec3.hpp"
#include <cmath>
#include "../util/log.hpp"

Vec3::Vec3(){
    set(0,0,0);
}

Vec3::Vec3(float xyz){
    set(xyz);
}

Vec3::Vec3(const Vec3& xyz){
    set(xyz.x,xyz.y,xyz.z);
}

Vec3::Vec3(Vec2 xy,float z){
    set(xy,z);
}

Vec3::Vec3(float x,float y,float z){
    set(x,y,z);
}

void Vec3::set(float xyz){
    set(xyz,xyz,xyz);
}

void Vec3::set(Vec3 xyz){
    set(xyz.x,xyz.y,xyz.z);
}

void Vec3::set(Vec2 xy,float z){
    set(xy.x,xy.y,z);
}

void Vec3::set(float newX,float newY,float newZ){
    x = newX;
    y = newY;
    z = newZ;
}

Vec3 Vec3::floor(){
    x = ::floor(x);
    y = ::floor(y);
    z = ::floor(z);
    return *this;
}

Vec3 Vec3::ceil(){
    x = ::ceil(x);
    y = ::ceil(y);
    z = ::ceil(z);
    return *this;
}

Vec3 Vec3::fract(){
    x = x-::floor(x);
    y = y-::floor(y);
    z = z-::floor(z);
    return *this;
}

float Vec3::length(){
    return sqrt(dot(*this,*this));
}

Vec3 Vec3::normalize(){
    *this *= 1.0f/length();
    return *this;
}

Vec3 Vec3::operator+=(Vec3 right){
    x += right.x;
    y += right.y;
    z += right.z;
    return *this;
}

Vec3 Vec3::operator+=(float right){
    x += right;
    y += right;
    z += right;
    return *this;
}

Vec3 Vec3::operator-=(Vec3 right){
    x -= right.x;
    y -= right.y;
    z -= right.z;
    return *this;
}

Vec3 Vec3::operator-=(float right){
    x -= right;
    y -= right;
    z -= right;
    return *this;
}

Vec3 Vec3::operator*=(Vec3 right){
    x *= right.x;
    y *= right.y;
    z *= right.z;
    return *this;
}

Vec3 Vec3::operator*=(float right){
    x *= right;
    y *= right;
    z *= right;
    return *this;
}

Vec3 Vec3::operator/=(Vec3 right){
    x /= right.x;
    y /= right.y;
    z /= right.z;
    return *this;
}

Vec3 Vec3::operator/=(float right){
    x /= right;
    y /= right;
    z /= right;
    return *this;
}

Vec3 Vec3::operator=(Vec3 right){
    set(right);
    return *this;
}

void Vec3::debug(){
    LOGD("Vec3(%f,%f,%f)",x,y,z);
}

float dot(Vec3 left,Vec3 right){
    return left.x*right.x+left.y*right.y+left.z*right.z;
}

Vec3 cross(Vec3 left,Vec3 right){
    Vec3 re(
        left.y*right.z-left.z*right.y,
        left.z*right.x-left.x*right.z,
        left.x*right.y-left.y*right.x
    );
    return re;
}

Vec3 operator+(Vec3 left,Vec3 right){
    Vec3 re(left.x+right.x,left.y+right.y,left.z+right.z);
    return re;
}

Vec3 operator+(Vec3 left,float right){
    Vec3 re(left.x+right,left.y+right,left.z+right);
    return re;
}

Vec3 operator-(Vec3 left,Vec3 right){
    Vec3 re(left.x-right.x,left.y-right.y,left.z-right.z);
    return re;
}

Vec3 operator-(Vec3& left,float right){
    Vec3 re(left.x-right,left.y-right,left.z-right);
    return re;
}

Vec3 operator*(Vec3 left,Vec3 right){
    Vec3 re(left.x*right.x,left.y*right.y,left.z*right.z);
    return re;
}

Vec3 operator*(Vec3 left,float right){
    Vec3 re(left.x*right,left.y*right,left.z*right);
    return re;
}

Vec3 operator*(float left,Vec3 right){
    return right*left;
}

Vec3 operator/(Vec3 left,Vec3 right){
    Vec3 re(left.x/right.x,left.y/right.y,left.z/right.z);
    return re;
}

Vec3 operator/(Vec3 left,float right){
    Vec3 re(left.x/right,left.y/right,left.z/right);
    return re;
}

bool operator==(Vec3 left,Vec3 right){
    return left.x==right.x&&left.y==right.y&&left.z==right.z;
}

bool operator!=(Vec3 left,Vec3 right){
    return !(left==right);
}

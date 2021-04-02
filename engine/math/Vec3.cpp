#include "math/Vec3.hpp"
#include "util/log.hpp"
#include <cmath>

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

Vec3 Vec3::operator*=(Mat3x3 right){
    *this = *this*right;
    return *this;
}

void Vec3::debug(){
    LOGD("Vec3(%f,%f,%f)",x,y,z);
}

float Vec3::dot(Vec3 left,Vec3 right){
    return left.x*right.x+left.y*right.y+left.z*right.z;
}

Vec3 Vec3::cross(Vec3 left,Vec3 right){
    return Vec3(
        left.y*right.z-left.z*right.y,
        left.z*right.x-left.x*right.z,
        left.x*right.y-left.y*right.x
    );
}

Vec3 Vec3::findOrthogonal(Vec3 v){
    if(fabs(v.x) < fabs(v.y) && fabs(v.x) < fabs(v.z))
        return cross(Vec3(1.0, 0.0, 0.0), v).normalize();
    if(fabs(v.y) < fabs(v.z))
        return cross(Vec3(0.0, 1.0, 0.0), v).normalize();
    return cross(Vec3(0.0, 0.0, 1.0), v).normalize();
}

Vec3 operator+(Vec3 left,Vec3 right){
    return Vec3(left.x+right.x,left.y+right.y,left.z+right.z);
}

Vec3 operator+(Vec3 left,float right){
    return Vec3(left.x+right,left.y+right,left.z+right);
}

Vec3 operator-(Vec3 left,Vec3 right){
    return Vec3(left.x-right.x,left.y-right.y,left.z-right.z);
}

Vec3 operator-(Vec3& left,float right){
    return Vec3(left.x-right,left.y-right,left.z-right);
}

Vec3 operator*(Vec3 left,Vec3 right){
    return Vec3(left.x*right.x,left.y*right.y,left.z*right.z);
}

Vec3 operator*(Vec3 left,float right){
    return Vec3(left.x*right,left.y*right,left.z*right);
}

Vec3 operator*(float left,Vec3 right){
    return right*left;
}

Vec3 operator/(Vec3 left,Vec3 right){
    return Vec3(left.x/right.x,left.y/right.y,left.z/right.z);
}

Vec3 operator/(Vec3 left,float right){
    return Vec3(left.x/right,left.y/right,left.z/right);
}

Vec3 operator-(Vec3 v){
    return Vec3(-v.x, -v.y, -v.z);
}

Vec3 operator+(Vec3 v){
    return v;
}

bool operator==(Vec3 left,Vec3 right){
    return left.x==right.x&&left.y==right.y&&left.z==right.z;
}

bool operator!=(Vec3 left,Vec3 right){
    return !(left==right);
}

Vec3 operator*(Vec3 left, Mat3x3 right){
    float *m = right.matrix;
    Vec3 re;
    re.x = m[0]*left.x + m[1]*left.y + m[2]*left.z;
    re.y = m[3]*left.x + m[4]*left.y + m[5]*left.z;
    re.z = m[6]*left.x + m[7]*left.y + m[8]*left.z;
    return re;
}

Vec3 operator*(Mat3x3 left, Vec3 right){
    float *m = left.matrix;
    Vec3 re;
    re.x = m[0]*right.x + m[3]*right.y + m[6]*right.z;
    re.y = m[1]*right.x + m[4]*right.y + m[7]*right.z;
    re.z = m[2]*right.x + m[5]*right.y + m[8]*right.z;
    return re;
}

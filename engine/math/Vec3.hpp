#ifndef HEADER_MATH_VEC3_H
#define HEADER_MATH_VEC3_H

#include "Vec2.hpp"

class Vec3{
    public:
        float x,y,z;
        Vec3();
        Vec3(const Vec3& xyz);
        Vec3(float xyz);
        Vec3(float x,float y,float z);
        Vec3(Vec2 xy,float z);
        void set(float x,float y,float z);
        void set(Vec3 xyz);
        void set(float xyz);
        void set(Vec2 xy,float z);
        void floor();
        void ceil();
        void fract();
        float length();
        void normalize();
        Vec3 operator+=(Vec3 right);
        Vec3 operator+=(float right);
        Vec3 operator-=(Vec3 right);
        Vec3 operator-=(float right);
        Vec3 operator*=(Vec3 right);
        Vec3 operator*=(float right);
        Vec3 operator/=(Vec3 right);
        Vec3 operator/=(float right);
        Vec3 operator=(Vec3 right);
        void debug();
};

float dot(Vec3 left,Vec3 right);
Vec3 cross(Vec3 left,Vec3 right);

Vec3 operator*(Vec3 left,Vec3 right);
Vec3 operator*(Vec3 left,float right);
Vec3 operator*(float left,Vec3 right);

Vec3 operator/(Vec3 left,Vec3 right);
Vec3 operator/(Vec3 left,float right);

Vec3 operator+(Vec3 left,Vec3 right);
Vec3 operator+(Vec3 left,float right);

Vec3 operator-(Vec3 left,Vec3 right);
Vec3 operator-(Vec3 left,float right);

bool operator==(Vec3 left,Vec3 right);
bool operator!=(Vec3 left,Vec3 right);

#endif

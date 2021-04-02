#pragma once

#include "math/Vec3.hpp"
#include "math/Mat4x4.hpp"

class Vec4{
    public:
        float x,y,z,w;
        Vec4();
        Vec4(float xyzw);
        Vec4(const Vec4& xyzw);
        Vec4(Vec3 xyz,float w);
        Vec4(Vec2 xy,float z,float w);
        Vec4(float x,float y,float z,float w);
        void set(float xyzw);
        void set(Vec4 xyzw);
        void set(Vec3 xyz,float w);
        void set(Vec2 xy,float z,float w);
        void set(float x,float y,float z,float w);
        void floor();
        void ceil();
        void fract();
        float length();
        void normalize();
        Vec3 xyz();
        Vec4 operator+=(Vec4 right);
        Vec4 operator+=(float right);
        Vec4 operator-=(Vec4 right);
        Vec4 operator-=(float right);
        Vec4 operator*=(Vec4 right);
        Vec4 operator*=(float right);
        Vec4 operator/=(Vec4 right);
        Vec4 operator/=(float right);
        Vec4 operator=(Vec4 right);
        Vec4 operator*=(Mat4x4 right);
        void debug();
        static float dot(Vec4 left,Vec4 right);
};

Vec4 operator+(Vec4 left,Vec4 right);
Vec4 operator+(Vec4 left,float right);

Vec4 operator*(Vec4 left,Vec4 right);
Vec4 operator*(Vec4 left,float right);
Vec4 operator*(float left,Vec4 right);

Vec4 operator/(Vec4 left,Vec4 right);
Vec4 operator/(Vec4 left,float right);

Vec4 operator-(Vec4 left,Vec4 right);
Vec4 operator-(Vec4 left,float right);

Vec4 operator-(Vec4 v);
Vec4 operator+(Vec4 v);

bool operator==(Vec4 left,Vec4 right);
bool operator!=(Vec4 left,Vec4 right);

Vec4 operator*(Vec4 left, Mat4x4 right);
Vec4 operator*(Mat4x4 left, Vec4 right);

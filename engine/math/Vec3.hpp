#pragma once

#include "math/Vec2.hpp"
#include "math/Mat3x3.hpp"

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
        Vec3 floor();
        Vec3 ceil();
        Vec3 fract();
        float length();
        Vec3 normalize();
        Vec3 operator+=(Vec3 right);
        Vec3 operator+=(float right);
        Vec3 operator-=(Vec3 right);
        Vec3 operator-=(float right);
        Vec3 operator*=(Vec3 right);
        Vec3 operator*=(float right);
        Vec3 operator/=(Vec3 right);
        Vec3 operator/=(float right);
        Vec3 operator=(Vec3 right);
        Vec3 operator*=(Mat3x3 right);
        void debug();
        static float dot(Vec3 left,Vec3 right);
        static Vec3 cross(Vec3 left,Vec3 right);
        static Vec3 findOrthogonal(Vec3 v);
};

Vec3 operator*(Vec3 left,Vec3 right);
Vec3 operator*(Vec3 left,float right);
Vec3 operator*(float left,Vec3 right);

Vec3 operator/(Vec3 left,Vec3 right);
Vec3 operator/(Vec3 left,float right);

Vec3 operator+(Vec3 left,Vec3 right);
Vec3 operator+(Vec3 left,float right);

Vec3 operator-(Vec3 left,Vec3 right);
Vec3 operator-(Vec3 left,float right);

Vec3 operator-(Vec3 v);
Vec3 operator+(Vec3 v);

bool operator==(Vec3 left,Vec3 right);
bool operator!=(Vec3 left,Vec3 right);

Vec3 operator*(Vec3 left, Mat3x3 right);
Vec3 operator*(Mat3x3 left, Vec3 right);

#pragma once

#include "math/Vec4.hpp"

#define F_PI 3.14159265358979323846264338328f

template<typename T>
struct VectorTraits {
    static const int length;
    static T construct(float f[length]);
};

template<>
struct VectorTraits<float> {
    static const int length = 1;
    static float contruct(float f[1]) { return f[0]; }
};

template<>
struct VectorTraits<Vec2> {
    static const int length = 2;
    static Vec2 construct(float f[2]) { return Vec2(f[0], f[1]); }
};

template<>
struct VectorTraits<Vec3> {
    static const int length = 3;
    static Vec3 construct(float f[3]) { return Vec3(f[0], f[1], f[2]); }
};

template<>
struct VectorTraits<Vec4> {
    static const int length = 4;
    static Vec4 construct(float f[4]) { return Vec4(f[0], f[1], f[2], f[3]); }
};

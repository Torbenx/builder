#pragma once

#include "math/Mat4x4.hpp"
#include "math/Vec3.hpp"
#include <algorithm>

class Vec4 {
public:
    union {
        struct {
            float x, y, z, w;
        };
        Vec2 xy;
        Vec3 xyz;
    };
    Vec4()
        : x(0.0f), y(0.0f), z(0.0f), w(0.0f) { }
    Vec4(float x, float y, float z, float w)
        : x(x), y(y), z(z), w(w) { }
    Vec4(Vec2 xy, float z, float w)
        : xy(xy) {
        this->z = z;
        this->w = w;
    }
    Vec4(Vec3 xyz, float w)
        : xyz(xyz) {
        this->w = w;
    }
    static Vec4 clampToW(Vec4 v) {
        return {
            std::clamp(v.x, -std::abs(v.w), std::abs(v.w)),
            std::clamp(v.y, -std::abs(v.w), std::abs(v.w)),
            std::clamp(v.z, -std::abs(v.w), std::abs(v.w))
        };
    }
    Vec4& operator*=(const Mat4x4& right);
    void debug() { log<Tag::Math>("Vec4({} ,{} ,{} ,{})", x, y, z, w); }
};

inline Vec4 operator*(Vec4 left, const Mat4x4& right) {
    const float* m = right.matrix;
    return {
        left.x * m[0] + left.y * m[1] + left.z * m[2] + left.w * m[3],
        left.x * m[4] + left.y * m[5] + left.z * m[6] + left.w * m[7],
        left.x * m[8] + left.y * m[9] + left.z * m[10] + left.w * m[11],
        left.x * m[12] + left.y * m[13] + left.z * m[14] + left.w * m[15]
    };
}

inline Vec4 operator*(const Mat4x4& left, Vec4 right) {
    const float* m = left.matrix;
    return {
        right.x * m[0] + right.y * m[4] + right.z * m[8] + right.w * m[12],
        right.x * m[1] + right.y * m[5] + right.z * m[9] + right.w * m[13],
        right.x * m[2] + right.y * m[6] + right.z * m[10] + right.w * m[14],
        right.x * m[3] + right.y * m[7] + right.z * m[11] + right.w * m[15]
    };
}

inline Vec4& Vec4::operator*=(const Mat4x4& right) { return *this = *this * right; }

#pragma once

#include "math/Mat3x3.hpp"
#include "math/Vec2.hpp"

class Vec3 {
public:
    union {
        struct {
            float x, y, z;
        };
        Vec2 xy;
    };
    Vec3()
        : x(0.0f), y(0.0f), z(0.0f) { }
    Vec3(float xyz)
        : x(xyz), y(xyz), z(xyz) { }
    Vec3(float x, float y, float z)
        : x(x), y(y), z(z) { }
    Vec3(Vec2 xy, float z)
        : xy(xy) { this->z = z; }
    float length() { return std::sqrt(x * x + y * y + z * z); }
    static Vec3 normalize(Vec3 v);
    Vec3& operator+=(Vec3 right);
    Vec3& operator+=(float right);
    Vec3& operator-=(Vec3 right);
    Vec3& operator-=(float right);
    Vec3& operator*=(Vec3 right);
    Vec3& operator*=(float right);
    Vec3& operator/=(Vec3 right);
    Vec3& operator/=(float right);
    Vec3& operator*=(const Mat3x3& right);
    void debug();
    static float dot(Vec3 left, Vec3 right);
    static Vec3 cross(Vec3 left, Vec3 right);
    static Vec3 findOrthogonal(Vec3 v);
};

inline float Vec3::dot(Vec3 left, Vec3 right) { return left.x * right.x + left.y * right.y + left.z * right.z; }

inline Vec3 operator+(Vec3 left, Vec3 right) { return { left.x + right.x, left.y + right.y, left.z + right.z }; }
inline Vec3 operator+(Vec3 left, float right) { return { left.x + right, left.y + right, left.z + right }; }

inline Vec3 operator-(Vec3 left, Vec3 right) { return { left.x - right.x, left.y - right.y, left.z - right.z }; }
inline Vec3 operator-(Vec3 left, float right) { return { left.x - right, left.y - right, left.z - right }; }

inline Vec3 operator*(Vec3 left, Vec3 right) { return { left.x * right.x, left.y * right.y, left.z * right.z }; }
inline Vec3 operator*(Vec3 left, float right) { return { left.x * right, left.y * right, left.z * right }; }
inline Vec3 operator*(float left, Vec3 right) { return right * left; }

inline Vec3 operator/(Vec3 left, Vec3 right) { return { left.x / right.x, left.y / right.y, left.z / right.z }; }
inline Vec3 operator/(Vec3 left, float right) { return { left.x / right, left.y / right, left.z / right }; }

inline Vec3 operator-(Vec3 v) { return { -v.x, -v.y, -v.z }; }
inline Vec3 operator+(Vec3 v) { return v; }

inline Vec3 operator*(Vec3 left, Mat3x3 right) {
    float* m = right.matrix;
    return {
        m[0] * left.x + m[1] * left.y + m[2] * left.z,
        m[3] * left.x + m[4] * left.y + m[5] * left.z,
        m[6] * left.x + m[7] * left.y + m[8] * left.z
    };
}
inline Vec3 operator*(Mat3x3 left, Vec3 right) {
    float* m = left.matrix;
    return {
        m[0] * right.x + m[3] * right.y + m[6] * right.z,
        m[1] * right.x + m[4] * right.y + m[7] * right.z,
        m[2] * right.x + m[5] * right.y + m[8] * right.z
    };
}

inline Vec3 Vec3::normalize(Vec3 v) {
    float inv = 1.0f / v.length();
    return v * inv;
}

inline Vec3 Vec3::cross(Vec3 left, Vec3 right) {
    return {
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

inline Vec3 Vec3::findOrthogonal(Vec3 v) {
    if (std::abs(v.x) < std::abs(v.y) && std::abs(v.x) < std::abs(v.z))
        return normalize(cross({ 1.0, 0.0, 0.0 }, v));
    if (std::abs(v.y) < std::abs(v.z))
        return normalize(cross({ 0.0, 1.0, 0.0 }, v));
    return normalize(cross({ 0.0, 0.0, 1.0 }, v));
}

inline Vec3& Vec3::operator+=(Vec3 v) { return *this = *this + v; }
inline Vec3& Vec3::operator+=(float f) { return *this = *this + f; }
inline Vec3& Vec3::operator-=(Vec3 v) { return *this = *this - v; }
inline Vec3& Vec3::operator-=(float f) { return *this = *this - f; }
inline Vec3& Vec3::operator*=(Vec3 v) { return *this = *this * v; }
inline Vec3& Vec3::operator*=(float f) { return *this = *this * f; }
inline Vec3& Vec3::operator*=(const Mat3x3& right) { return *this = *this * right; }
inline Vec3& Vec3::operator/=(Vec3 v) { return *this = *this / v; }
inline Vec3& Vec3::operator/=(float f) { return *this = *this / f; }

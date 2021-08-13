#pragma once

#include "util/log.hpp"
#include <cmath>

namespace Tag {
struct Math {
    static constexpr char tag[] = "math";
    static constexpr bool verifyEnabled = true;
};
}

class Vec2 {
public:
    float x, y;
    Vec2()
        : x(0.0f), y(0.0f) { }
    Vec2(float xy)
        : x(xy), y(xy) { }
    Vec2(float x, float y)
        : x(x), y(y) { }
    void set(float x, float y) {
        this->x = x;
        this->y = y;
    }
    void set(float xy) {
        x = xy;
        y = xy;
    }
    static Vec2 floor(Vec2 v) { return { std::floor(v.x), std::floor(v.y) }; }
    static Vec2 ceil(Vec2 v) { return { std::ceil(v.x), std::ceil(v.y) }; }
    static Vec2 fract(Vec2 v);
    float length() { return std::sqrt(x * x + y * y); }
    static Vec2 normalize(Vec2 v);
    Vec2& operator+=(Vec2 v);
    Vec2& operator+=(float f);
    Vec2& operator-=(Vec2 v);
    Vec2& operator-=(float f);
    Vec2& operator*=(Vec2 v);
    Vec2& operator*=(float f);
    Vec2& operator/=(Vec2 v);
    Vec2& operator/=(float f);
    void debug() { log<Tag::Math>("Vec2({}, {})", x, y); }
    static float dot(Vec2 v1, Vec2 v2) { return v1.x * v2.x + v1.y * v2.y; }
};

inline Vec2 operator+(Vec2 v1, Vec2 v2) { return { v1.x + v2.x, v1.y + v2.y }; }
inline Vec2 operator+(Vec2 v, float f) { return { v.x + f, v.y + f }; }

inline Vec2 operator*(Vec2 v1, Vec2 v2) { return { v1.x * v2.x, v1.y * v2.y }; }
inline Vec2 operator*(Vec2 v, float f) { return { v.x * f, v.y * f }; }
inline Vec2 operator*(float f, Vec2 v) { return { v.x * f, v.y * f }; }

inline Vec2 operator/(Vec2 left, Vec2 right) { return { left.x / right.x, left.y / right.y }; }
inline Vec2 operator/(Vec2 left, float right) { return left * (1.0f / right); }

inline Vec2 operator-(Vec2 v1, Vec2 v2) { return { v1.x - v2.x, v1.y - v2.y }; }
inline Vec2 operator-(Vec2 v, float f) { return { v.x - f, v.y - f }; }

inline Vec2 operator-(Vec2 v) { return { -v.x, -v.y }; }
inline Vec2 operator+(Vec2 v) { return v; }

inline Vec2& Vec2::operator+=(Vec2 v) { return *this = *this + v; }
inline Vec2& Vec2::operator+=(float f) { return *this = *this + f; }
inline Vec2& Vec2::operator-=(Vec2 v) { return *this = *this - v; }
inline Vec2& Vec2::operator-=(float f) { return *this = *this - f; }
inline Vec2& Vec2::operator*=(Vec2 v) { return *this = *this * v; }
inline Vec2& Vec2::operator*=(float f) { return *this = *this * f; }
inline Vec2& Vec2::operator/=(Vec2 v) { return *this = *this / v; }
inline Vec2& Vec2::operator/=(float f) { return *this = *this / f; }

inline Vec2 Vec2::normalize(Vec2 v) {
    float inv = 1.0f / v.length();
    return v * inv;
}

inline Vec2 Vec2::fract(Vec2 v) { return v - Vec2::floor(v); }

#include "math/Vec4.hpp"
#include "math/constants.hpp"
#include "util/log.hpp"
#include <algorithm>
#include <cmath>

Vec4::Vec4() {
    set(0, 0, 0, 0);
}

Vec4::Vec4(float xyzw) {
    set(xyzw);
}

Vec4::Vec4(const Vec4& xyzw) {
    set(xyzw.x, xyzw.y, xyzw.z, xyzw.w);
}

Vec4::Vec4(Vec3 xyz, float w) {
    set(xyz, w);
}

Vec4::Vec4(Vec2 xy, float z, float w) {
    set(xy, z, w);
}

Vec4::Vec4(float x, float y, float z, float w) {
    set(x, y, z, w);
}

void Vec4::set(float xyzw) {
    set(xyzw, xyzw, xyzw, xyzw);
}

void Vec4::set(Vec4 xyzw) {
    set(xyzw.x, xyzw.y, xyzw.z, xyzw.w);
}

void Vec4::set(Vec3 xyz, float w) {
    set(xyz.x, xyz.y, xyz.z, w);
}

void Vec4::set(Vec2 xy, float z, float w) {
    set(xy.x, xy.y, z, w);
}

void Vec4::set(float newX, float newY, float newZ, float newW) {
    x = newX;
    y = newY;
    z = newZ;
    w = newW;
}

void Vec4::floor() {
    x = ::floor(x);
    y = ::floor(y);
    z = ::floor(z);
    w = ::floor(w);
}

void Vec4::ceil() {
    x = ::ceil(x);
    y = ::ceil(y);
    z = ::ceil(z);
    w = ::ceil(w);
}

void Vec4::fract() {
    x = x - ::floor(x);
    y = y - ::floor(y);
    z = z - ::floor(z);
    w = w - ::floor(w);
}

float Vec4::length() {
    return sqrt(dot(*this, *this));
}

void Vec4::normalize() {
    *this *= 1.0f / length();
}

void Vec4::clampToW() {
    x = std::clamp(x, -std::abs(w), std::abs(w));
    y = std::clamp(y, -std::abs(w), std::abs(w));
    z = std::clamp(z, -std::abs(w), std::abs(w));
}

Vec3 Vec4::xyz() {
    return Vec3(x, y, z);
}

Vec4 Vec4::operator+=(Vec4 right) {
    x += right.x;
    y += right.y;
    z += right.z;
    w += right.w;
    return *this;
}

Vec4 Vec4::operator+=(float right) {
    x += right;
    y += right;
    z += right;
    w += right;
    return *this;
}

Vec4 Vec4::operator-=(Vec4 right) {
    x -= right.x;
    y -= right.y;
    z -= right.z;
    w -= right.w;
    return *this;
}

Vec4 Vec4::operator-=(float right) {
    x -= right;
    y -= right;
    z -= right;
    w -= right;
    return *this;
}

Vec4 Vec4::operator*=(Vec4 right) {
    x *= right.x;
    y *= right.y;
    z *= right.z;
    w *= right.w;
    return *this;
}

Vec4 Vec4::operator*=(float right) {
    x *= right;
    y *= right;
    z *= right;
    w *= right;
    return *this;
}

Vec4 Vec4::operator/=(Vec4 right) {
    x /= right.x;
    y /= right.y;
    z /= right.z;
    w /= right.w;
    return *this;
}

Vec4 Vec4::operator/=(float right) {
    x /= right;
    y /= right;
    z /= right;
    w /= right;
    return *this;
}

Vec4 Vec4::operator=(Vec4 right) {
    set(right);
    return *this;
}

Vec4 Vec4::operator*=(Mat4x4 right) {
    *this = *this * right;
    return *this;
}

void Vec4::debug() {
    log<Tag::Math>("Vec4({} ,{} ,{} ,{} )", x, y, z, w);
}

float Vec4::dot(Vec4 left, Vec4 right) {
    return left.x * right.x + left.y * right.y + left.z * right.z + left.w * right.w;
}

Vec4 operator+(Vec4 left, Vec4 right) {
    return Vec4(left.x + right.x, left.y + right.y, left.z + right.z, left.w + right.w);
}

Vec4 operator+(Vec4 left, float right) {
    return Vec4(left.x + right, left.y + right, left.z + right, left.w + right);
}

Vec4 operator-(Vec4 left, Vec4 right) {
    return Vec4(left.x - right.x, left.y - right.y, left.z - right.z, left.w - right.w);
}

Vec4 operator-(Vec4 left, float right) {
    return Vec4(left.x - right, left.y - right, left.z - right, left.w - right);
}

Vec4 operator*(Vec4 left, Vec4 right) {
    return Vec4(left.x * right.x, left.y * right.y, left.z * right.z, left.w * right.w);
}

Vec4 operator*(Vec4 left, float right) {
    return Vec4(left.x * right, left.y * right, left.z * right, left.w * right);
}

Vec4 operator*(float left, Vec4 right) {
    return right * left;
}

Vec4 operator/(Vec4 left, Vec4 right) {
    return Vec4(left.x / right.x, left.y / right.y, left.z / right.z, left.w / right.w);
}

Vec4 operator/(Vec4 left, float right) {
    return Vec4(left.x / right, left.y / right, left.z / right, left.w / right);
}

Vec4 operator-(Vec4 v) {
    return Vec4(-v.x, -v.y, -v.z, -v.z);
}

Vec4 operator+(Vec4 v) {
    return v;
}

bool operator==(Vec4 left, Vec4 right) {
    return left.x == right.x && left.y == right.y && left.z == right.z && left.w == right.w;
}

bool operator!=(Vec4 left, Vec4 right) {
    return !(left == right);
}

Vec4 operator*(Vec4 left, Mat4x4 right) {
    Vec4 re;
    float* m = right.matrix;
    re.x = left.x * m[0] + left.y * m[1] + left.z * m[2] + left.w * m[3];
    re.y = left.x * m[4] + left.y * m[5] + left.z * m[6] + left.w * m[7];
    re.z = left.x * m[8] + left.y * m[9] + left.z * m[10] + left.w * m[11];
    re.w = left.x * m[12] + left.y * m[13] + left.z * m[14] + left.w * m[15];
    return re;
}

Vec4 operator*(Mat4x4 left, Vec4 right) {
    Vec4 re;
    float* m = left.matrix;
    re.x = right.x * m[0] + right.y * m[4] + right.z * m[8] + right.w * m[12];
    re.y = right.x * m[1] + right.y * m[5] + right.z * m[9] + right.w * m[13];
    re.z = right.x * m[2] + right.y * m[6] + right.z * m[10] + right.w * m[14];
    re.w = right.x * m[3] + right.y * m[7] + right.z * m[11] + right.w * m[15];
    return re;
}

#pragma once

#include <cmath>

namespace newmath {

struct CNumber;
struct Vec2;
struct Surface2;
struct Rotation2;
struct Transform2;
struct Vec3;
struct Surface3;
struct Rotation3;
struct Transform3;
struct PVec3;

struct CNumber {
    float r = 0.0f;
    float i = 0.0f;
    constexpr CNumber();
    constexpr CNumber(float, float);
    explicit constexpr CNumber(float);
    explicit constexpr CNumber(const Vec2&);
    constexpr CNumber& operator*=(const CNumber&);
    constexpr CNumber& operator*=(float);
    constexpr CNumber& operator/=(const CNumber&);
    constexpr CNumber& operator/=(float);
    constexpr CNumber& operator+=(const CNumber&);
    constexpr CNumber& operator+=(float);
    constexpr CNumber& operator-=(const CNumber&);
    constexpr CNumber& operator-=(float);
};
constexpr CNumber operator*(const CNumber&, const CNumber&);
constexpr CNumber operator*(float, const CNumber&);
constexpr CNumber operator*(const CNumber&, float);
constexpr CNumber operator/(const CNumber&, const CNumber&);
constexpr CNumber operator/(float, const CNumber&);
constexpr CNumber operator/(const CNumber&, float);
constexpr CNumber operator+(const CNumber&, const CNumber&);
constexpr CNumber operator+(float, const CNumber&);
constexpr CNumber operator+(const CNumber&, float);
constexpr CNumber operator-(const CNumber&, const CNumber&);
constexpr CNumber operator-(float, const CNumber&);
constexpr CNumber operator-(const CNumber&, float);
constexpr CNumber operator-(const CNumber&);
constexpr CNumber conjugate(const CNumber&);
constexpr float length(const CNumber&);
constexpr float lengthSq(const CNumber&);
constexpr float invLength(const CNumber&);
constexpr float invLengthSq(const CNumber&);
constexpr CNumber inverse(const CNumber&);
constexpr CNumber expI(float);

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
    constexpr Vec2();
    constexpr Vec2(float, float);
    explicit constexpr Vec2(const CNumber&);
    constexpr Vec2& operator*=(float);
    constexpr Vec2& operator/=(float);
    constexpr Vec2& operator+=(const Vec2&);
    constexpr Vec2& operator-=(const Vec2&);
};
constexpr float operator*(const Vec2&, const Vec2&);
constexpr Vec2 operator*(float, const Vec2&);
constexpr Vec2 operator*(const Vec2&, float);
constexpr Vec2 operator/(const Vec2&, float);
constexpr Vec2 operator+(const Vec2&, const Vec2&);
constexpr Vec2 operator-(const Vec2&, const Vec2&);
constexpr Vec2 operator-(const Vec2&);
constexpr float length(const Vec2&);
constexpr float lengthSq(const Vec2&);
constexpr float invLength(const Vec2&);
constexpr float invLengthSq(const Vec2&);
constexpr Vec2 normalize(const Vec2&);
constexpr Vec2 quarterTurn(const Vec2&);

struct Surface2 {
    union {
        Vec2 normal {};
        struct {
            float x, y;
        };
    };
    float w = 0.0f;
    constexpr Surface2();
    constexpr Surface2(const Vec2& point, const Vec2& normal);
    constexpr Surface2(const Vec2&, float);
    static constexpr Surface2 fromPoints(const Vec2&, const Vec2&);
    constexpr Surface2& operator*=(float);
};
constexpr float operator*(const Surface2&, const Vec2&);
constexpr Surface2 operator*(float, const Surface2&);
constexpr Surface2 operator*(const Surface2&, float);
constexpr Surface2 operator-(const Surface2&);
constexpr Surface2 normalize(const Surface2&);

struct Rotation2 {
    std::array<float, 4> matrix = { 1.0f, /*bottom-left*/ 0.0f, /*top-right*/ 0.0f, 1.0f };
    constexpr Rotation2();
    explicit constexpr Rotation2(const CNumber&);
    explicit constexpr Rotation2(const std::array<float, 4>&);
    constexpr Rotation2& operator*=(const Rotation2&);
    constexpr Vec2 operator()(const Vec2&) const;
    constexpr Vec2 operator[](const Vec2&) const;
};
constexpr Rotation2 operator*(const Rotation2&, const Rotation2&);
constexpr Surface2 operator*(const Surface2&, const Rotation2&);
constexpr Rotation2 inverse(const Rotation2&);
constexpr Rotation2 rotate(float);

struct Transform2 {
    Rotation2 rotation {};
    Vec2 translation {};
    constexpr Transform2();
    constexpr Transform2(const Rotation2&, const Vec2&);
    constexpr Transform2(const Rotation2&);
    explicit constexpr Transform2(const Vec2&);
    constexpr Transform2& operator*=(const Transform2&);
    constexpr Transform2& operator*=(const Rotation2&);
    constexpr Vec2 operator()(const Vec2&) const;
    constexpr Vec2 operator[](const Vec2&) const;
};
constexpr Transform2 operator*(const Transform2&, const Transform2&);
constexpr Transform2 operator*(const Rotation2&, const Transform2&);
constexpr Transform2 operator*(const Transform2&, const Rotation2&);
constexpr Surface2 operator*(const Surface2&, const Transform2&);
constexpr Transform2 inverse(const Transform2&);
constexpr Transform2 translate(const Vec2&);
constexpr Transform2 rotateAbout(const Vec2&, float);
constexpr Transform2 reflectAbout(const Surface2&);

struct Vec3 {
    union {
        struct {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
        };
        Vec2 xy;
    };
    constexpr Vec3();
    constexpr Vec3(float, float, float);
    constexpr Vec3(const Vec2&, float);
    constexpr Vec3& operator*=(float);
    constexpr Vec3& operator/=(float);
    constexpr Vec3& operator+=(const Vec3&);
    constexpr Vec3& operator-=(const Vec3&);
};
constexpr float operator*(const Vec3&, const Vec3&);
constexpr Vec3 operator*(float, const Vec3&);
constexpr Vec3 operator*(const Vec3&, float);
constexpr Vec3 operator/(const Vec3&, float);
constexpr Vec3 operator+(const Vec3&, const Vec3&);
constexpr Vec3 operator-(const Vec3&, const Vec3&);
constexpr Vec3 operator-(const Vec3&);
constexpr float length(const Vec3&);
constexpr float lengthSq(const Vec3&);
constexpr float invLength(const Vec3&);
constexpr float invLengthSq(const Vec3&);
constexpr Vec3 normalize(const Vec3&);
constexpr Vec3 cross(const Vec3&, const Vec3&);

struct Surface3 {
    union {
        Vec3 normal {};
        struct {
            float x, y, z;
        };
    };
    float w = 0.0f;
    constexpr Surface3();
    constexpr Surface3(const Vec3& point, const Vec3& normal);
    constexpr Surface3(const Vec3&, float);
    static constexpr Surface3 fromPoints(const Vec3&, const Vec3&, const Vec3&);
    constexpr Surface3& operator*=(float);
};
constexpr float operator*(const Surface3&, const Vec3&);
constexpr Surface3 operator*(float, const Surface3&);
constexpr Surface3 operator*(const Surface3&, float);
constexpr Surface3 operator-(const Surface3&);
constexpr Surface3 normalize(const Surface3&);

struct Rotation3 {
    std::array<float, 9> matrix = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    constexpr Rotation3();
    explicit constexpr Rotation3(const std::array<float, 9>&);
    constexpr Rotation3& operator*=(const Rotation3&);
    constexpr Vec3 operator()(const Vec3&) const;
    constexpr Vec3 operator[](const Vec3&) const;
};
constexpr Rotation3 operator*(const Rotation3&, const Rotation3&);
constexpr Surface3 operator*(const Surface3&, const Rotation3&);
constexpr Rotation3 inverse(const Rotation3&);
constexpr Rotation3 rotate(const Vec3&, float);

struct Transform3 {
    Rotation3 rotation {};
    Vec3 translation {};
    constexpr Transform3();
    constexpr Transform3(const Rotation3&, const Vec3&);
    constexpr Transform3(const Rotation3&);
    explicit constexpr Transform3(const Vec3&);
    constexpr Transform3& operator*=(const Transform3&);
    constexpr Transform3& operator*=(const Rotation3&);
    constexpr Vec3 operator()(const Vec3&) const;
    constexpr Vec3 operator[](const Vec3&) const;
};
constexpr Transform3 operator*(const Transform3&, const Transform3&);
constexpr Transform3 operator*(const Rotation3&, const Transform3&);
constexpr Transform3 operator*(const Transform3&, const Rotation3&);
constexpr Surface3 operator*(const Surface3&, const Transform3&);
constexpr Transform3 inverse(const Transform3&);
constexpr Transform3 translate(const Vec3&);
constexpr Transform3 rotateAbout(const Vec3&, const Vec3&, float);
constexpr Transform3 reflectAbout(const Surface3&);

struct PVec3 {
    union {
        Vec3 xyz {};
        struct {
            float x, y, z;
        };
    };
    float w = 1.0f;
    constexpr PVec3();
    explicit constexpr PVec3(const Vec3&);
    constexpr PVec3(const Vec3&, float);
    constexpr PVec3& operator*=(float);
    constexpr PVec3& operator/=(float);
    constexpr PVec3& operator+=(const Vec3&);
    constexpr PVec3& operator-=(const Vec3&);
};
constexpr float operator*(const Surface3&, const PVec3&);
constexpr PVec3 operator*(const Transform3&, const PVec3&);
constexpr PVec3 operator*(const Rotation3&, const PVec3&);
constexpr PVec3 operator*(float, const PVec3&);
constexpr PVec3 operator*(const PVec3&, float);
constexpr PVec3 operator/(const PVec3&, float);
constexpr PVec3 operator+(const PVec3&, const Vec3&);
constexpr PVec3 operator+(const Vec3&, const PVec3&);
constexpr PVec3 operator-(const PVec3&, const Vec3&);
constexpr PVec3 operator-(const PVec3&);
constexpr PVec3 normalize(const PVec3&);

struct ScreenProjection {
    float near = 1.0f; // = N
    float c = 1.0f; // = 1/(1-N/F)
    float invWidth = 0.5f; // = 1/W
    float invHeight = 0.5f; // = 1/H
    /* 2N/W     0     0     0
     *    0  2N/H     0     0
     *    0     0  2c-1  -2Nc
     *    0     0     1     0
     */
    constexpr float far() const { return near * c / (c - 1.0f); }
};

constexpr CNumber::CNumber() { }
constexpr CNumber::CNumber(float r, float i)
    : r(r), i(i) { }
constexpr CNumber::CNumber(float r)
    : r(r) { }
constexpr CNumber::CNumber(const Vec2& v)
    : r(v.x), i(v.y) { }
constexpr CNumber& CNumber::operator*=(const CNumber& f) { return (*this) = (*this) * f; }
constexpr CNumber& CNumber::operator*=(float f) { return (*this) = (*this) * f; }
constexpr CNumber& CNumber::operator/=(const CNumber& f) { return (*this) = (*this) / f; }
constexpr CNumber& CNumber::operator/=(float f) { return (*this) = (*this) / f; }
constexpr CNumber& CNumber::operator+=(const CNumber& f) { return (*this) = (*this) + f; }
constexpr CNumber& CNumber::operator+=(float f) { return (*this) = (*this) + f; }
constexpr CNumber& CNumber::operator-=(const CNumber& f) { return (*this) = (*this) - f; }
constexpr CNumber& CNumber::operator-=(float f) { return (*this) = (*this) - f; }
constexpr CNumber operator*(const CNumber& l, const CNumber& r) { return { l.r * r.r - l.i * r.i, l.r * r.i + l.i * r.i }; }
constexpr CNumber operator*(float l, const CNumber& r) { return { l * r.r, l * r.i }; }
constexpr CNumber operator*(const CNumber& l, float r) { return { l.r * r, l.i * r }; }
constexpr CNumber operator/(const CNumber& l, const CNumber& r) { return l * conjugate(r) * invLength(r); }
constexpr CNumber operator/(float l, const CNumber& r) { return { l * conjugate(r) * invLength(r) }; }
constexpr CNumber operator/(const CNumber& l, float r) { return { l.r / r, l.i / r }; }
constexpr CNumber operator+(const CNumber& l, const CNumber& r) { return { l.r + r.r, l.i + r.i }; }
constexpr CNumber operator+(float l, const CNumber& r) { return { l + r.r, r.i }; }
constexpr CNumber operator+(const CNumber& l, float r) { return { l.r + r, l.i }; }
constexpr CNumber operator-(const CNumber& l, const CNumber& r) { return { l.r - r.r, l.i - l.i }; }
constexpr CNumber operator-(float l, const CNumber& r) { return { l - r.r, -r.i }; }
constexpr CNumber operator-(const CNumber& l, float r) { return { l.r - r, l.i }; }
constexpr CNumber operator-(const CNumber& z) { return { -z.r, -z.i }; }
constexpr CNumber conjugate(const CNumber& z) { return { z.r, -z.i }; }
constexpr float length(const CNumber& z) { return std::sqrt(lengthSq(z)); }
constexpr float lengthSq(const CNumber& z) { return z.r * z.r + z.i * z.i; }
constexpr float invLength(const CNumber& z) { return 1.0f / length(z); }
constexpr float invLengthSq(const CNumber& z) { return 1.0f / lengthSq(z); }
constexpr CNumber inverse(const CNumber& z) { return conjugate(z) * invLength(z); }
constexpr CNumber expI(float rads) { return { std::cos(rads), std::sin(rads) }; }

constexpr Vec2::Vec2() { }
constexpr Vec2::Vec2(float x, float y)
    : x(x), y(y) { }
constexpr Vec2::Vec2(const CNumber& z)
    : x(z.r), y(z.i) { }
constexpr Vec2& Vec2::operator*=(float f) { return (*this) = (*this) * f; }
constexpr Vec2& Vec2::operator/=(float f) { return (*this) = (*this) / f; }
constexpr Vec2& Vec2::operator+=(const Vec2& f) { return (*this) = (*this) + f; }
constexpr Vec2& Vec2::operator-=(const Vec2& f) { return (*this) = (*this) - f; }
constexpr float operator*(const Vec2& l, const Vec2& r) { return l.x * r.x + l.y * r.y; }
constexpr Vec2 operator*(float f, const Vec2& v) { return { v.x * f, v.y * f }; }
constexpr Vec2 operator*(const Vec2& v, float f) { return { v.x * f, v.y * f }; }
constexpr Vec2 operator/(const Vec2& v, float f) { return { v.x / f, v.y / f }; }
constexpr Vec2 operator+(const Vec2& l, const Vec2& r) { return { l.x + r.x, l.y + r.y }; }
constexpr Vec2 operator-(const Vec2& l, const Vec2& r) { return { l.x - r.x, l.y - r.y }; }
constexpr Vec2 operator-(const Vec2& v) { return { -v.x, -v.y }; }
constexpr float length(const Vec2& v) { return std::sqrt(lengthSq(v)); }
constexpr float lengthSq(const Vec2& v) { return v.x * v.x + v.y * v.y; }
constexpr float invLength(const Vec2& v) { return 1.0f / length(v); }
constexpr float invLengthSq(const Vec2& v) { return 1.0f / lengthSq(v); }
constexpr Vec2 normalize(const Vec2& v) { return v * invLength(v); }
constexpr Vec2 quarterTurn(const Vec2& v) { return { -v.y, v.x }; }

constexpr Vec3::Vec3() { }
constexpr Vec3::Vec3(float x, float y, float z)
    : x(x), y(y), z(z) { }
constexpr Vec3::Vec3(const Vec2& xy, float z)
    : x(xy.x), y(xy.y), z(z) { }
constexpr Vec3& Vec3::operator*=(float f) { return (*this) = (*this) * f; }
constexpr Vec3& Vec3::operator/=(float f) { return (*this) = (*this) / f; }
constexpr Vec3& Vec3::operator+=(const Vec3& f) { return (*this) = (*this) + f; }
constexpr Vec3& Vec3::operator-=(const Vec3& f) { return (*this) = (*this) - f; }
constexpr float operator*(const Vec3& l, const Vec3& r) { return l.x * r.x + l.y * r.y + l.z * r.z; }
constexpr Vec3 operator*(float f, const Vec3& v) { return { v.x * f, v.y * f, v.z * f }; }
constexpr Vec3 operator*(const Vec3& v, float f) { return { v.x * f, v.y * f, v.z * f }; }
constexpr Vec3 operator/(const Vec3& v, float f) { return { v.x / f, v.y / f, v.z / f }; }
constexpr Vec3 operator+(const Vec3& l, const Vec3& r) { return { l.x + r.x, l.y + r.y, l.z + r.z }; }
constexpr Vec3 operator-(const Vec3& l, const Vec3& r) { return { l.x - r.x, l.y - r.y, l.z - r.z }; }
constexpr Vec3 operator-(const Vec3& v) { return { -v.x, -v.y, -v.z }; }
constexpr float length(const Vec3& v) { return std::sqrt(lengthSq(v)); }
constexpr float lengthSq(const Vec3& v) { return v.x * v.x + v.y * v.y + v.z * v.z; }
constexpr float invLength(const Vec3& v) { return 1.0f / length(v); }
constexpr float invLengthSq(const Vec3& v) { return 1.0f / lengthSq(v); }
constexpr Vec3 normalize(const Vec3& v) { return v * invLength(v); }
constexpr Vec3 cross(const Vec3& l, const Vec3& r) { return { l.y * r.z - l.z * r.y, l.z * r.x - l.x * r.z, l.x * r.y - l.y * r.x }; }

constexpr Surface2::Surface2() { }
constexpr Surface2::Surface2(const Vec2& point, const Vec2& normal)
    : normal(normal), w(-normal * point) { }
constexpr Surface2::Surface2(const Vec2& normal, float w)
    : normal(normal), w(w) { }
constexpr Surface2 Surface2::fromPoints(const Vec2& p1, const Vec2& p2) {
    Vec2 normal = quarterTurn(p2 - p1);
    return { normal, -normal * p1 };
}
constexpr Surface2& Surface2::operator*=(float f) { return (*this) = (*this) * f; }
constexpr float operator*(const Surface2& s, const Vec2& v) { return s.normal * v + s.w; }
constexpr Surface2 operator*(float f, const Surface2& s) { return { s.normal * f, s.w * f }; }
constexpr Surface2 operator*(const Surface2& s, float f) { return { s.normal * f, s.w * f }; }
constexpr Surface2 operator-(const Surface2& s) { return { -s.normal, -s.w }; }
constexpr Surface2 normalize(const Surface2& s) { return s * invLength(s.normal); }

constexpr Surface3::Surface3() { }
constexpr Surface3::Surface3(const Vec3& point, const Vec3& normal)
    : normal(normal), w(-normal * point) { }
constexpr Surface3::Surface3(const Vec3& normal, float w)
    : normal(normal), w(w) { }
constexpr Surface3 Surface3::fromPoints(const Vec3& p1, const Vec3& p2, const Vec3& p3) {
    Vec3 normal = cross(p3 - p1, p2 - p1);
    return { normal, -normal * p1 };
}
constexpr Surface3& Surface3::operator*=(float f) { return (*this) = (*this) * f; }
constexpr float operator*(const Surface3& s, const Vec3& v) { return s.normal * v + s.w; }
constexpr Surface3 operator*(float f, const Surface3& s) { return { s.normal * f, s.w * f }; }
constexpr Surface3 operator*(const Surface3& s, float f) { return { s.normal * f, s.w * f }; }
constexpr Surface3 operator-(const Surface3& s) { return { -s.normal, -s.w }; }
constexpr Surface3 normalize(const Surface3& s) { return s * invLength(s.normal); }

constexpr Rotation2::Rotation2() { }
constexpr Rotation2::Rotation2(const CNumber& z)
    : matrix { z.r, /*bottom-left*/ z.i, /*top-right*/ -z.i, z.r } { }
constexpr Rotation2::Rotation2(const std::array<float, 4>& matrix)
    : matrix(matrix) { }
constexpr Rotation2& Rotation2::operator*=(const Rotation2& r) { return (*this) = (*this) * r; }
constexpr Vec2 Rotation2::operator()(const Vec2& v) const { return { matrix[0] * v.x + matrix[2] * v.y, matrix[1] * v.x + matrix[3] * v.y }; }
constexpr Vec2 Rotation2::operator[](const Vec2& v) const { return { matrix[0] * v.x + matrix[2] * v.y, matrix[1] * v.x + matrix[3] * v.y }; }
constexpr Rotation2 operator*(const Rotation2& l, const Rotation2& r) {
    return Rotation2 { {
        l.matrix[0] * r.matrix[0] + l.matrix[2] * r.matrix[1],
        l.matrix[1] * r.matrix[0] + l.matrix[3] * r.matrix[1],
        l.matrix[0] * r.matrix[2] + l.matrix[2] * r.matrix[3],
        l.matrix[1] * r.matrix[2] + l.matrix[3] * r.matrix[3],
    } };
}
constexpr Surface2 operator*(const Surface2& s, const Rotation2& r) { return { inverse(r)[s.normal], s.w }; }
constexpr Rotation2 inverse(const Rotation2& r) { return Rotation2 { { r.matrix[0], r.matrix[2], r.matrix[1], r.matrix[3] } }; }
constexpr Rotation2 rotate(float rads) { return Rotation2 { expI(rads) }; }

constexpr Rotation3::Rotation3() { }
constexpr Rotation3::Rotation3(const std::array<float, 9>& matrix)
    : matrix(matrix) { }
constexpr Rotation3& Rotation3::operator*=(const Rotation3& r) { return (*this) = (*this) * r; }
constexpr Vec3 Rotation3::operator()(const Vec3& v) const {
    return {
        matrix[0] * v.x + matrix[3] * v.y + matrix[6] * v.z,
        matrix[1] * v.x + matrix[4] * v.y + matrix[7] * v.z,
        matrix[2] * v.x + matrix[5] * v.y + matrix[8] * v.z,
    };
}
constexpr Vec3 Rotation3::operator[](const Vec3& v) const {
    return {
        matrix[0] * v.x + matrix[3] * v.y + matrix[6] * v.z,
        matrix[1] * v.x + matrix[4] * v.y + matrix[7] * v.z,
        matrix[2] * v.x + matrix[5] * v.y + matrix[8] * v.z,
    };
}
constexpr Rotation3 operator*(const Rotation3& l, const Rotation3& r) {
    return Rotation3 { {
        l.matrix[0] * r.matrix[0] + l.matrix[3] * r.matrix[1] + l.matrix[6] * r.matrix[2],
        l.matrix[1] * r.matrix[0] + l.matrix[4] * r.matrix[1] + l.matrix[7] * r.matrix[2],
        l.matrix[2] * r.matrix[0] + l.matrix[5] * r.matrix[1] + l.matrix[8] * r.matrix[2],
        l.matrix[0] * r.matrix[3] + l.matrix[3] * r.matrix[4] + l.matrix[6] * r.matrix[5],
        l.matrix[1] * r.matrix[3] + l.matrix[4] * r.matrix[4] + l.matrix[7] * r.matrix[5],
        l.matrix[2] * r.matrix[3] + l.matrix[5] * r.matrix[4] + l.matrix[8] * r.matrix[5],
        l.matrix[0] * r.matrix[6] + l.matrix[3] * r.matrix[7] + l.matrix[6] * r.matrix[8],
        l.matrix[1] * r.matrix[6] + l.matrix[4] * r.matrix[7] + l.matrix[7] * r.matrix[8],
        l.matrix[2] * r.matrix[6] + l.matrix[5] * r.matrix[7] + l.matrix[8] * r.matrix[8],
    } };
}
constexpr Surface3 operator*(const Surface3& s, const Rotation3& r) { return { inverse(r)[s.normal], s.w }; }
constexpr Rotation3 inverse(const Rotation3& r) {
    // clang-format off
    return Rotation3 { {
        r.matrix[0], r.matrix[3], r.matrix[6],
        r.matrix[1], r.matrix[4], r.matrix[7],
        r.matrix[2], r.matrix[5], r.matrix[8],
    } };
    // clang-format on
}
constexpr Rotation3 rotate(const Vec3& n, float rads) {
    // VERIFY_NORMALIZED(n);
    // clang-format off
    CNumber c = expI(rads);
    float f = 1.0f - c.r;
    return Rotation3 { {
        f * n.x * n.x + c.r      , f * n.y * n.x + c.i * n.z, f * n.z * n.x - c.i * n.y,
        f * n.x * n.y - c.i * n.z, f * n.y * n.y + c.r      , f * n.z * n.y + c.i * n.x,
        f * n.x * n.z + c.i * n.y, f * n.y * n.z - c.i * n.x, f * n.z * n.z + c.r      ,
    } };
    // clang-format on
}

constexpr Transform2::Transform2() { }
constexpr Transform2::Transform2(const Rotation2& r, const Vec2& t)
    : rotation(r), translation(t) { }
constexpr Transform2::Transform2(const Rotation2& r)
    : rotation(r) { }
constexpr Transform2::Transform2(const Vec2& t)
    : translation(t) { }
constexpr Transform2& Transform2::operator*=(const Transform2& f) { return (*this) = (*this) * f; }
constexpr Transform2& Transform2::operator*=(const Rotation2& f) { return (*this) = (*this) * f; }
constexpr Vec2 Transform2::operator()(const Vec2& v) const { return rotation(v) + translation; }
constexpr Vec2 Transform2::operator[](const Vec2& v) const { return rotation[v]; }
constexpr Transform2 operator*(const Transform2& l, const Transform2& r) { return { l.rotation * r.rotation, l.translation + l.rotation[r.translation] }; }
constexpr Transform2 operator*(const Rotation2& l, const Transform2& r) { return { l * r.rotation, l[r.translation] }; }
constexpr Transform2 operator*(const Transform2& l, const Rotation2& r) { return { l.rotation * r, l.translation }; }
constexpr Surface2 operator*(const Surface2& s, const Transform2& t) {
    Vec2 normal = inverse(t.rotation)[s.normal];
    return { normal, s.w - normal * t.translation };
}
constexpr Transform2 inverse(const Transform2& t) {
    Rotation2 rotation = inverse(t.rotation);
    return { rotation, -rotation[t.translation] };
}
constexpr Transform2 translate(const Vec2& v) { return Transform2 { v }; }
constexpr Transform2 rotateAbout(const Vec2& p, float rads) { return translate(p) * rotate(rads) * translate(-p); }
constexpr Transform2 reflectAbout(const Surface2& s) {
    // VERIFY_NORMALIZED(s);
    Rotation2 ref { { 1.0f - 2.0f * s.x * s.x, -2.0f * s.x * s.y, -2.0f * s.y * s.x, 1.0f - 2.0f * s.y * s.y } };
    return translate(s.normal * s.w) * ref * translate(-s.normal * s.w);
}

constexpr Transform3::Transform3() { }
constexpr Transform3::Transform3(const Rotation3& r, const Vec3& t)
    : rotation(r), translation(t) { }
constexpr Transform3::Transform3(const Rotation3& r)
    : rotation(r) { }
constexpr Transform3::Transform3(const Vec3& t)
    : translation(t) { }
constexpr Transform3& Transform3::operator*=(const Transform3& f) { return (*this) = (*this) * f; }
constexpr Transform3& Transform3::operator*=(const Rotation3& f) { return (*this) = (*this) * f; }
constexpr Vec3 Transform3::operator()(const Vec3& v) const { return rotation(v) + translation; }
constexpr Vec3 Transform3::operator[](const Vec3& v) const { return rotation[v]; }
constexpr Transform3 operator*(const Transform3& l, const Transform3& r) { return { l.rotation * r.rotation, l.translation + l.rotation[r.translation] }; }
constexpr Transform3 operator*(const Rotation3& l, const Transform3& r) { return { l * r.rotation, l[r.translation] }; }
constexpr Transform3 operator*(const Transform3& l, const Rotation3& r) { return { l.rotation * r, l.translation }; }
constexpr Surface3 operator*(const Surface3& s, const Transform3& t) {
    Vec3 normal = inverse(t.rotation)[s.normal];
    return { normal, s.w - normal * t.translation };
}
constexpr Transform3 inverse(const Transform3& t) {
    Rotation3 rotation = inverse(t.rotation);
    return { rotation, -rotation[t.translation] };
}
constexpr Transform3 translate(const Vec3& v) { return Transform3 { v }; }
constexpr Transform3 rotateAbout(const Vec3& p, const Vec3& n, float rads) { return translate(p) * rotate(n, rads) * translate(-p); }
constexpr Transform3 reflectAbout(const Surface3& s) {
    // VERIFY_NORMALIZED(s);
    // clang-format off
    Rotation3 ref { {
        1.0f - 2.0f * s.x * s.x,      - 2.0f * s.y * s.x,      - 2.0f * s.z * s.x,
             - 2.0f * s.x * s.y, 1.0f - 2.0f * s.y * s.y,      - 2.0f * s.z * s.y,
             - 2.0f * s.x * s.z,      - 2.0f * s.y * s.z, 1.0f - 2.0f * s.z * s.z,
    } };
    // clang-format on
    return translate(s.normal * s.w) * ref * translate(-s.normal * s.w);
}

constexpr PVec3::PVec3() { }
constexpr PVec3::PVec3(const Vec3& v)
    : xyz(v) { }
constexpr PVec3::PVec3(const Vec3& v, float w)
    : xyz(v), w(w) { }
constexpr PVec3& PVec3::operator*=(float f) { return (*this) = (*this) * f; }
constexpr PVec3& PVec3::operator/=(float f) { return (*this) = (*this) / f; }
constexpr PVec3& PVec3::operator+=(const Vec3& f) { return (*this) = (*this) + f; }
constexpr PVec3& PVec3::operator-=(const Vec3& f) { return (*this) = (*this) - f; }
constexpr float operator*(const Surface3& s, const PVec3& p) { return s.x * p.x + s.y * p.y + s.z * p.z + s.w * p.w; }
constexpr PVec3 operator*(const Transform3& t, const PVec3& p) { return t.rotation * p + t.translation; }
constexpr PVec3 operator*(const Rotation3& r, const PVec3& p) { return { r[p.xyz], p.w }; }
constexpr PVec3 operator*(float f, const PVec3& p) { return { p.xyz * f, p.w * f }; }
constexpr PVec3 operator*(const PVec3& p, float f) { return { p.xyz * f, p.w * f }; }
constexpr PVec3 operator/(const PVec3& p, float f) { return { p.xyz / f, p.w / f }; }
constexpr PVec3 operator+(const PVec3& p, const Vec3& v) { return { p.xyz + v * p.w, p.w }; }
constexpr PVec3 operator+(const Vec3& v, const PVec3& p) { return { p.xyz + v * p.w, p.w }; }
constexpr PVec3 operator-(const PVec3& p, const Vec3& v) { return { p.xyz - v * p.w, p.w }; }
constexpr PVec3 operator-(const PVec3& p) { return { -p.xyz, -p.w }; }
constexpr PVec3 normalize(const PVec3& p) { return { p.xyz / p.w, 1.0f }; }

}

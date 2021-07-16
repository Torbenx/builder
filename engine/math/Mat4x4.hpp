#pragma once

#include "math/Vec3.hpp"

/*

  Format:
  m[ 0]  m[ 4]  m[ 8]  m[12]
  m[ 1]  m[ 5]  m[ 9]  m[13]
  m[ 2]  m[ 6]  m[10]  m[14]
  m[ 3]  m[ 7]  m[11]  m[15]

*/

class Mat4x4{
public:
    static Mat4x4 Id;
    float matrix[16];
    Mat4x4();
    Mat4x4(bool id);
    static Mat4x4 translate(Vec3 v);
    static Mat4x4 look(Vec3 eye, Vec3 direction, Vec3 up);
    static Mat4x4 lookAt(Vec3 eye, Vec3 center, Vec3 up);
    static Mat4x4 rotate(float radians, Vec3 axis);
    static Mat4x4 rotateAbout(Vec3 pos, float radians, Vec3 axis);
    static Mat4x4 reflection(Vec3 normal, Vec3 pos);
    static Mat4x4 frustum(float left, float right, float bottom, float top, float _near, float _far);
    static Mat4x4 frustumVH(float vfov, float hfov, float _near, float _far);
    static Mat4x4 frustumV(float vfov, float aspect, float _near, float _far);
    static Mat4x4 frustumH(float hfov, float aspect, float _near, float _far);
    static Mat4x4 ortho(float left, float right, float bottom, float top, float _near, float _far);
    static Mat4x4 invert(Mat4x4 matrix);
    static Mat4x4 diagonal(Vec3 v);
    Mat4x4& operator*=(Mat4x4 right);
    Mat4x4& operator*=(float right);
    void debug();
};

Mat4x4 operator*(Mat4x4 left, Mat4x4 right);

Mat4x4 operator*(Mat4x4 left, float right);
Mat4x4 operator*(float left, Mat4x4 right);

#pragma once

#include "math/Vec2.hpp"

/*

  Format:
  m[0]  m[3]  m[6]
  m[1]  m[4]  m[7]
  m[2]  m[5]  m[8]

*/

class Mat3x3{
    public:
        float matrix[9];
        static Mat3x3 Id;
        Mat3x3();
        Mat3x3(bool id);
        static Mat3x3 rotate(float radians);
        static Mat3x3 rotateAbout(Vec2 c, float radians);
        static Mat3x3 translate(Vec2 v);
        static Mat3x3 reflection(Vec2 normal, Vec2 pos);
        static Mat3x3 diagonal(Vec2 v);
        static Mat3x3 invert(Mat3x3 matrix);
        Mat3x3 operator*=(Mat3x3 right);
        Mat3x3 operator*=(float right);
        Mat3x3 operator=(Mat3x3 right);
        void debug();
};

Mat3x3 operator*(Mat3x3 left, Mat3x3 right);

Mat3x3 operator*(Mat3x3 left, float right);
Mat3x3 operator*(float left, Mat3x3 right);

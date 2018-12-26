#ifndef HEADER_MATH_MAT4X4_H
#define HEADER_MATH_MAT4X4_H

#include "Vec3.hpp"

class Mat4x4{
    public:
        static Mat4x4 Id;
        float matrix[16];
        Mat4x4();
        Mat4x4(bool id);
        ~Mat4x4();
        void translate(Vec3 v);
        void translateRaw(Vec3 v);
        void setIdentity();
        void setLookAt(Vec3 eye,Vec3 center,Vec3 up);
        void setRotate(float radians,Vec3 v);
        void setReflection(Vec3 r,Vec3 pos);
        void setFrustum(float left,float right,float top,float bottom,float near,float far);
        void setFrustumVH(float vfov,float hfov,float near,float far);
        void setFrustumV(float vfov,float aspect,float near,float far);
        void setFrustumH(float hfov,float aspect,float near,float far);
        void setOrtho(float left,float right,float bottom,float top,float near,float far);
        void scale(Vec3 scale);
        void invert();
        Mat4x4 operator*=(Mat4x4 right);
        Mat4x4 operator=(Mat4x4 right);
        void debug();
        static Mat4x4 wrap(float* m);
};

Mat4x4 operator*(Mat4x4 left,Mat4x4 right);

#endif

#include "math/Mat3x3.hpp"
#include "util/log.hpp"
#include <cmath>

Mat3x3 Mat3x3::Id(true);

Mat3x3::Mat3x3(bool id){
    if(id){
        float *m = matrix;
        m[0] = 1.0f;
        m[1] = 0.0f;
        m[2] = 0.0f;
        m[3] = 0.0f;
        m[4] = 1.0f;
        m[5] = 0.0f;
        m[6] = 0.0f;
        m[7] = 0.0f;
        m[8] = 1.0f;
    }
}

Mat3x3::Mat3x3(){

}

Mat3x3 Mat3x3::rotate(float radians){
    Mat3x3 re;
    float *m = re.matrix;
    float c = cos(radians);
    float s = sin(radians);

    m[0] = c;
    m[1] = s;
    m[2] = 0.0f;

    m[3] = -s;
    m[4] = c;
    m[5] = 0.0f;

    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 1.0f;
    return re;
}

Mat3x3 Mat3x3::rotateAbout(Vec2 c, float radians){
    return translate(c) * rotate(radians) * translate(-c);
}

Mat3x3 Mat3x3::translate(Vec2 v){
    Mat3x3 re(true);
    float *m = re.matrix;
    m[6] = v.x;
    m[7] = v.y;
    return re;
}

Mat3x3 Mat3x3::reflection(Vec2 n, Vec2 pos){
    Mat3x3 re;
    float *m = re.matrix;
    m[0] = 1.0f-2.0f*n.x*n.x;
    m[1] =     -2.0f*n.x*n.x;
    m[2] = 0.0f;

    m[3] =     -2.0f*n.y*n.x;
    m[4] = 1.0f-2.0f*n.y*n.y;
    m[5] = 0.0f;

    float l = 2.0f*Vec2::dot(n,pos);
    m[6] = l*n.x;
    m[7] = l*n.y;
    m[8] = 1.0f;
    return re;
}

Mat3x3 Mat3x3::diagonal(Vec2 v){
    Mat3x3 re;
    float *m = re.matrix;
    m[0] = v.x;
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = v.y;
    m[5] = 0.0f;
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 1.0f;
    return re;
}

Mat3x3 Mat3x3::invert(Mat3x3 matrix){
    log<Tag::Math>("invert not implemented");
    return Mat3x3();
}

Mat3x3 Mat3x3::operator*=(Mat3x3 right){
    *this = *this*right;
    return *this;
}

Mat3x3 Mat3x3::operator*=(float right){
    float *m = matrix;
    m[0] *= right;
    m[1] *= right;
    m[2] *= right;
    m[3] *= right;
    m[4] *= right;
    m[5] *= right;
    m[6] *= right;
    m[7] *= right;
    m[8] *= right;
    return *this;
}

Mat3x3 Mat3x3::operator=(Mat3x3 right){
    float *m0 = matrix;
    float *m1 = right.matrix;
    m0[0] = m1[0];
    m0[1] = m1[1];
    m0[2] = m1[2];
    m0[3] = m1[3];
    m0[4] = m1[4];
    m0[5] = m1[5];
    m0[6] = m1[6];
    m0[7] = m1[7];
    m0[8] = m1[8];
    return *this;
}

void Mat3x3::debug(){
    float *m = matrix;
    log<Tag::Math>("{} {} {}\n{} {} {}\n{} {} {}",
        m[0], m[3], m[6],
        m[1], m[4], m[7],
        m[2], m[5], m[8]);
}

Mat3x3 operator*(Mat3x3 left, Mat3x3 right){
    Mat3x3 re;
    float *m0 = re.matrix;
    float *m1 = left.matrix;
    float *m2 = right.matrix;
    m0[0] = m1[0]*m2[0] + m1[3]*m2[1] + m1[6]*m2[2];
    m0[1] = m1[1]*m2[0] + m1[4]*m2[1] + m1[7]*m2[2];
    m0[2] = m1[2]*m2[0] + m1[5]*m2[1] + m1[8]*m2[2];

    m0[3] = m1[0]*m2[3] + m1[3]*m2[4] + m1[6]*m2[5];
    m0[4] = m1[1]*m2[3] + m1[4]*m2[4] + m1[7]*m2[5];
    m0[5] = m1[2]*m2[3] + m1[5]*m2[4] + m1[8]*m2[5];

    m0[6] = m1[0]*m2[6] + m1[3]*m2[7] + m1[3]*m2[8];
    m0[7] = m1[1]*m2[6] + m1[4]*m2[7] + m1[4]*m2[8];
    m0[8] = m1[1]*m2[6] + m1[5]*m2[7] + m1[5]*m2[8];

    return re;
}

Mat3x3 operator*(Mat3x3 left, float right){
    Mat3x3 re;
    float *m0 = re.matrix;
    float *m1 = left.matrix;
    m0[0] = m1[0] * right;
    m0[1] = m1[1] * right;
    m0[2] = m1[2] * right;
    m0[3] = m1[3] * right;
    m0[4] = m1[4] * right;
    m0[5] = m1[5] * right;
    m0[6] = m1[6] * right;
    m0[7] = m1[7] * right;
    m0[8] = m1[8] * right;
    return re;
}

Mat3x3 operator*(float left, Mat3x3 right){
    return right*left;
}

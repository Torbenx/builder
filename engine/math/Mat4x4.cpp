#include "Mat4x4.hpp"
#include <cmath>
#include "../util/log.hpp"

Mat4x4 Mat4x4::Id(true);

Mat4x4::Mat4x4(){
    
}

Mat4x4::Mat4x4(bool id){
    if(id){
        float *m = matrix;
        m[ 0] = 1.0f;
        m[ 1] = 0.0f;
        m[ 2] = 0.0f;
        m[ 3] = 0.0f;
        
        m[ 4] = 0.0f;
        m[ 5] = 1.0f;
        m[ 6] = 0.0f;
        m[ 7] = 0.0f;
        
        m[ 8] = 0.0f;
        m[ 9] = 0.0f;
        m[10] = 1.0f;
        m[11] = 0.0f;
        
        m[12] = 0.0f;
        m[13] = 0.0f;
        m[14] = 0.0f;
        m[15] = 1.0f;
    }
}

Mat4x4 Mat4x4::translate(Vec3 v){
    Mat4x4 re(true);
    float *m = re.matrix;
    m[12] = v.x;
    m[13] = v.y;
    m[14] = v.z;
    return re;
}

Mat4x4 Mat4x4::look(Vec3 eye, Vec3 dir, Vec3 up){
    Mat4x4 re;
    float *m = re.matrix;
    dir.normalize();
    Vec3 ex = Vec3::cross(dir,up);
    ex.normalize();
    Vec3 ey = Vec3::cross(ex,dir); //already unit length
    
    m[ 0] = ex.x;
    m[ 1] = ey.x;
    m[ 2] = -dir.x;
    m[ 3] = 0.0f;
    
    m[ 4] = ex.y;
    m[ 5] = ey.y;
    m[ 6] = -dir.y;
    m[ 7] = 0.0f;
    
    m[ 8] = ex.z;
    m[ 9] = ey.z;
    m[10] = -dir.z;
    m[11] = 0.0f;
    
    m[12] = -Vec3::dot(ex,eye);
    m[13] = -Vec3::dot(ey,eye);
    m[14] = Vec3::dot(dir,eye);
    m[15] = 1.0f;
    return re;
}

Mat4x4 Mat4x4::lookAt(Vec3 eye, Vec3 center, Vec3 up){
    return look(eye, center-eye, up);
}

Mat4x4 Mat4x4::rotate(float radians, Vec3 axis){
    Mat4x4 re;
    float *m = re.matrix;
    float c = cos(radians);
    float s = sin(radians);
    float nc = 1.0f-c;
    Vec3 v = axis;
    v.normalize();
    
    m[ 0] = v.x * v.x * nc +       c;
    m[ 1] = v.y * v.x * nc + v.y * s;
    m[ 2] = v.z * v.x * nc - v.y * s;
    m[ 3] = 0.0f;
    
    m[ 4] = v.x * v.y * nc - v.z * s;
    m[ 5] = v.y * v.y * nc +       c;
    m[ 6] = v.z * v.y * nc + v.x * s;
    m[ 7] = 0.0f;
    
    m[ 8] = v.x * v.z * nc + v.y * s;
    m[ 9] = v.y * v.z * nc - v.x * s;
    m[10] = v.z * v.z * nc +       c;
    m[11] = 0.0f;
    
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    return re;
}

Mat4x4 Mat4x4::reflection(Vec3 n, Vec3 pos){
    Mat4x4 re;
    float *m = re.matrix;
    
    n.normalize();
    
    m[ 0] = 1.0f-2.0f*n.x*n.x;
    m[ 1] =     -2.0f*n.x*n.y;
    m[ 2] =     -2.0f*n.x*n.z;
    m[ 3] = 0.0f;
    
    m[ 4] =     -2.0f*n.y*n.x;
    m[ 5] = 1.0f-2.0f*n.y*n.y;
    m[ 6] =     -2.0f*n.y*n.z;
    m[ 7] = 0.0f;
    
    m[ 8] =     -2.0f*n.z*n.x;
    m[ 9] =     -2.0f*n.z*n.y;
    m[10] = 1.0f-2.0f*n.z*n.z;
    m[11] = 0.0f;
    
    float l = 2.0f*Vec3::dot(n,pos);
    m[12] = l*n.x;
    m[13] = l*n.y;
    m[14] = l*n.z;
    m[15] = 1.0f;
    return re;
}

Mat4x4 Mat4x4::frustum(float left, float right, float bottom, float top, float near, float far){
    Mat4x4 re;
    float *m = re.matrix;
    float r_width  = 1.0f / (right - left);
    float r_height = 1.0f / (top - bottom);
    float r_depth  = 1.0f / (near - far);
    float x = 2.0f * (near * r_width);
    float y = 2.0f * (near * r_height);
    float a = (right + left) * r_width;
    float b = (top + bottom) * r_height;
    float c = (far + near) * r_depth;
    float d = 2.0f * (far * near * r_depth);
    
    m[ 0] = x;
    m[ 1] = 0.0f;
    m[ 2] = 0.0f;
    m[ 3] = 0.0f;
    
    m[ 4] = 0.0f;
    m[ 5] = y;
    m[ 6] = 0.0f;
    m[ 7] = 0.0f;
    
    m[ 8] = a;
    m[ 9] = b;
    m[10] = c;
    m[11] = -1.0f;
    
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = d;
    m[15] = 0.0f;
    return re;
}

Mat4x4 Mat4x4::frustumVH(float vfov, float hfov, float near, float far){
    hfov = tan(hfov/2.0f);
    vfov = tan(vfov/2.0f);
    return frustum(-vfov,vfov,-hfov,hfov,near,far);
}
        
Mat4x4 Mat4x4::frustumV(float vfov, float aspect, float near, float far){
    vfov = tan(vfov/2.0f);
    return frustum(-vfov,vfov,-vfov*aspect,vfov*aspect,near,far);
}
        
Mat4x4 Mat4x4::frustumH(float hfov, float aspect, float near, float far){
    hfov = tan(hfov/2.0f);
    return frustum(-hfov*aspect,hfov*aspect,-hfov,hfov,near,far);
}

Mat4x4 Mat4x4::ortho(float left, float right, float bottom, float top, float near, float far){
    Mat4x4 re;
    float *m = re.matrix;
    
    m[ 0] = 2.0f/(right-left);
    m[ 1] = 0.0f;
    m[ 2] = 0.0f;
    m[ 3] = 0.0f;
    
    m[ 4] = 0.0f;
    m[ 5] = 2.0f/(top-bottom);
    m[ 6] = 0.0f;
    m[ 7] = 0.0f;
    
    m[ 8] = 0.0f;
    m[ 9] = 0.0f;
    m[10] = -2.0f/(far-near);
    m[11] = 0.0f;
    
    m[12] = -(right+left)/(right-left);
    m[13] = -(top+bottom)/(top-bottom);
    m[14] = -(far+near)/(far-near);
    m[15] = 1.0f;
    
    return re;
}

Mat4x4 Mat4x4::invert(Mat4x4 matrix){
    float *m = matrix.matrix;

    float  src0 = m[ 0];
    float  src4 = m[ 1];
    float  src8 = m[ 2];
    float src12 = m[ 3];

    float  src1 = m[ 4];
    float  src5 = m[ 5];
    float  src9 = m[ 6];
    float src13 = m[ 7];

    float  src2 = m[ 8];
    float  src6 = m[ 9];
    float src10 = m[10];
    float src14 = m[11];

    float  src3 = m[12];
    float  src7 = m[13];
    float src11 = m[14];
    float src15 = m[15];

    float atmp0  = src10 * src15;
    float atmp1  = src11 * src14;
    float atmp2  = src9  * src15;
    float atmp3  = src11 * src13;
    float atmp4  = src9  * src14;
    float atmp5  = src10 * src13;
    float atmp6  = src8  * src15;
    float atmp7  = src11 * src12;
    float atmp8  = src8  * src14;
    float atmp9  = src10 * src12;
    float atmp10 = src8  * src13;
    float atmp11 = src9  * src12;

    float dst0  = (atmp0 * src5 + atmp3 * src6 + atmp4  * src7)
                - (atmp1 * src5 + atmp2 * src6 + atmp5  * src7);
    float dst1  = (atmp1 * src4 + atmp6 * src6 + atmp9  * src7)
                - (atmp0 * src4 + atmp7 * src6 + atmp8  * src7);
    float dst2  = (atmp2 * src4 + atmp7 * src5 + atmp10 * src7)
                - (atmp3 * src4 + atmp6 * src5 + atmp11 * src7);
    float dst3  = (atmp5 * src4 + atmp8 * src5 + atmp11 * src6)
                - (atmp4 * src4 + atmp9 * src5 + atmp10 * src6);
    float dst4  = (atmp1 * src1 + atmp2 * src2 + atmp5  * src3)
                - (atmp0 * src1 + atmp3 * src2 + atmp4  * src3);
    float dst5  = (atmp0 * src0 + atmp7 * src2 + atmp8  * src3)
                - (atmp1 * src0 + atmp6 * src2 + atmp9  * src3);
    float dst6  = (atmp3 * src0 + atmp6 * src1 + atmp11 * src3)
                - (atmp2 * src0 + atmp7 * src1 + atmp10 * src3);
    float dst7  = (atmp4 * src0 + atmp9 * src1 + atmp10 * src2)
                - (atmp5 * src0 + atmp8 * src1 + atmp11 * src2);
                
    float btmp0  = src2 * src7;
    float btmp1  = src3 * src6;
    float btmp2  = src1 * src7;
    float btmp3  = src3 * src5;
    float btmp4  = src1 * src6;
    float btmp5  = src2 * src5;
    float btmp6  = src0 * src7;
    float btmp7  = src3 * src4;
    float btmp8  = src0 * src6;
    float btmp9  = src2 * src4;
    float btmp10 = src0 * src5;
    float btmp11 = src1 * src4;

    float dst8  = (btmp0  * src13 + btmp3  * src14 + btmp4  * src15)
                - (btmp1  * src13 + btmp2  * src14 + btmp5  * src15);
    float dst9  = (btmp1  * src12 + btmp6  * src14 + btmp9  * src15)
                - (btmp0  * src12 + btmp7  * src14 + btmp8  * src15);
    float dst10 = (btmp2  * src12 + btmp7  * src13 + btmp10 * src15)
                - (btmp3  * src12 + btmp6  * src13 + btmp11 * src15);
    float dst11 = (btmp5  * src12 + btmp8  * src13 + btmp11 * src14)
                - (btmp4  * src12 + btmp9  * src13 + btmp10 * src14);
    float dst12 = (btmp2  * src10 + btmp5  * src11 + btmp1  * src9 )
                - (btmp4  * src11 + btmp0  * src9  + btmp3  * src10);
    float dst13 = (btmp8  * src11 + btmp0  * src8  + btmp7  * src10)
                - (btmp6  * src10 + btmp9  * src11 + btmp1  * src8 );
    float dst14 = (btmp6  * src9  + btmp11 * src11 + btmp3  * src8 )
                - (btmp10 * src11 + btmp2  * src8  + btmp7  * src9 );
    float dst15 = (btmp10 * src10 + btmp4  * src8  + btmp9  * src9 )
                - (btmp8  * src9  + btmp11 * src10 + btmp5  * src8 );
                
    float det = src0 * dst0 + src1 * dst1 + src2 * dst2 + src3 * dst3;
    float invdet = 1.0f / det;

    Mat4x4 re;
    m = re.matrix;
    m[ 0] =  dst0*invdet;
    m[ 1] =  dst1*invdet;
    m[ 2] =  dst2*invdet;
    m[ 3] =  dst3*invdet;

    m[ 4] =  dst4*invdet;
    m[ 5] =  dst5*invdet;
    m[ 6] =  dst6*invdet;
    m[ 7] =  dst7*invdet;

    m[ 8] =  dst8*invdet;
    m[ 9] =  dst9*invdet;
    m[10] = dst10*invdet;
    m[11] = dst11*invdet;

    m[12] = dst12*invdet;
    m[13] = dst13*invdet;
    m[14] = dst14*invdet;
    m[15] = dst15*invdet;
    
    return re;
}

Mat4x4 Mat4x4::diagonal(Vec3 v){
    Mat4x4 re;
    float *m = re.matrix;
    m[ 0] = v.x;
    m[ 1] = 0.0f;
    m[ 2] = 0.0f;
    m[ 3] = 0.0f;
    m[ 4] = 0.0f;
    m[ 5] = v.y;
    m[ 6] = 0.0f;
    m[ 7] = 0.0f;
    m[ 8] = 0.0f;
    m[ 9] = 0.0f;
    m[10] = v.z;
    m[11] = 0.0f;
    m[12] = 0.0f;
    m[13] = 0.0f;
    m[14] = 0.0f;
    m[15] = 1.0f;
    return re;
}

Mat4x4 Mat4x4::operator*=(Mat4x4 right){
    *this = *this*right;
    return *this;
}

Mat4x4 Mat4x4::operator*=(float right){
    float *m = matrix;
    m[ 0] *= right;
    m[ 1] *= right;
    m[ 2] *= right;
    m[ 3] *= right;
    m[ 4] *= right;
    m[ 5] *= right;
    m[ 6] *= right;
    m[ 7] *= right;
    m[ 8] *= right;
    m[ 9] *= right;
    m[10] *= right;
    m[11] *= right;
    m[12] *= right;
    m[13] *= right;
    m[14] *= right;
    m[15] *= right;
    return *this;
}

Mat4x4 Mat4x4::operator=(Mat4x4 right){
    float *m1 = matrix;
    float *m2 = right.matrix;
    m1[ 0] = m2[ 0];
    m1[ 1] = m2[ 1];
    m1[ 2] = m2[ 2];
    m1[ 3] = m2[ 3];
    m1[ 4] = m2[ 4];
    m1[ 5] = m2[ 5];
    m1[ 6] = m2[ 6];
    m1[ 7] = m2[ 7];
    m1[ 8] = m2[ 8];
    m1[ 9] = m2[ 9];
    m1[10] = m2[10];
    m1[11] = m2[11];
    m1[12] = m2[12];
    m1[13] = m2[13];
    m1[14] = m2[14];
    m1[15] = m2[15];
    return *this;
}

void Mat4x4::debug(){
    float *m = matrix;
    LOGD("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
        m[ 0],m[ 4],m[ 8],m[12],
        m[ 1],m[ 5],m[ 9],m[13],
        m[ 2],m[ 6],m[10],m[14],
        m[ 3],m[ 7],m[11],m[15]
    );
}

Mat4x4 operator*(Mat4x4 left,Mat4x4 right){
    Mat4x4 re;
    float *m0 = re.matrix,*m1 = right.matrix,*m2 = left.matrix;
    
    m0[ 0] = m1[ 0] * m2[ 0] + m1[ 1] * m2[ 4] + m1[ 2] * m2[ 8] + m1[ 3] * m2[12];
    m0[ 1] = m1[ 0] * m2[ 1] + m1[ 1] * m2[ 5] + m1[ 2] * m2[ 9] + m1[ 3] * m2[13];
    m0[ 2] = m1[ 0] * m2[ 2] + m1[ 1] * m2[ 6] + m1[ 2] * m2[10] + m1[ 3] * m2[14];
    m0[ 3] = m1[ 0] * m2[ 3] + m1[ 1] * m2[ 7] + m1[ 2] * m2[11] + m1[ 3] * m2[15];
    
    m0[ 4] = m1[ 4] * m2[ 0] + m1[ 5] * m2[ 4] + m1[ 6] * m2[ 8] + m1[ 7] * m2[12];
    m0[ 5] = m1[ 4] * m2[ 1] + m1[ 5] * m2[ 5] + m1[ 6] * m2[ 9] + m1[ 7] * m2[13];
    m0[ 6] = m1[ 4] * m2[ 2] + m1[ 5] * m2[ 6] + m1[ 6] * m2[10] + m1[ 7] * m2[14];
    m0[ 7] = m1[ 4] * m2[ 3] + m1[ 5] * m2[ 7] + m1[ 6] * m2[11] + m1[ 7] * m2[15];
    
    m0[ 8] = m1[ 8] * m2[ 0] + m1[ 9] * m2[ 4] + m1[10] * m2[ 8] + m1[11] * m2[12];
    m0[ 9] = m1[ 8] * m2[ 1] + m1[ 9] * m2[ 5] + m1[10] * m2[ 9] + m1[11] * m2[13];
    m0[10] = m1[ 8] * m2[ 2] + m1[ 9] * m2[ 6] + m1[10] * m2[10] + m1[11] * m2[14];
    m0[11] = m1[ 8] * m2[ 3] + m1[ 9] * m2[ 7] + m1[10] * m2[11] + m1[11] * m2[15];
    
    m0[12] = m1[12] * m2[ 0] + m1[13] * m2[ 4] + m1[14] * m2[ 8] + m1[15] * m2[12];
    m0[13] = m1[12] * m2[ 1] + m1[13] * m2[ 5] + m1[14] * m2[ 9] + m1[15] * m2[13];
    m0[14] = m1[12] * m2[ 2] + m1[13] * m2[ 6] + m1[14] * m2[10] + m1[15] * m2[14];
    m0[15] = m1[12] * m2[ 3] + m1[13] * m2[ 7] + m1[14] * m2[11] + m1[15] * m2[15];
    
    return re;
}

Mat4x4 operator*(Mat4x4 left, float right){
    Mat4x4 re;
    float *m1 = re.matrix;
    float *m2 = left.matrix;
    m1[ 0] = m2[ 0] * right;
    m1[ 1] = m2[ 1] * right;
    m1[ 2] = m2[ 2] * right;
    m1[ 3] = m2[ 3] * right;
    m1[ 4] = m2[ 4] * right;
    m1[ 5] = m2[ 5] * right;
    m1[ 6] = m2[ 6] * right;
    m1[ 7] = m2[ 7] * right;
    m1[ 8] = m2[ 8] * right;
    m1[ 9] = m2[ 9] * right;
    m1[10] = m2[10] * right;
    m1[11] = m2[11] * right;
    m1[12] = m2[12] * right;
    m1[13] = m2[13] * right;
    m1[14] = m2[14] * right;
    m1[15] = m2[15] * right;
    return re;
}

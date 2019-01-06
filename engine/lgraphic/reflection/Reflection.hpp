#ifndef HEADER_LGRAPHIC_REFLECTION_REFLECTION_H
#define HEADER_LGRAPHIC_REFLECTION_REFLECTION_H

#include "../../graphic/Framebuffer.hpp"
#include "../../math/MvpMatrix.hpp"

class Reflection{
    public:
        Framebuffer framebuffer;
        Renderbuffer renderbuffer;
        Texture texture;
        Vec3 reflection,position;
        float factor;
        int width,height;
        Reflection(Vec3 reflection,Vec3 position,int width,int height,float factor);
        void init();
        void bind();
        void unbind();
        void getReflectionModel(Mat4x4& model);
};

#endif
